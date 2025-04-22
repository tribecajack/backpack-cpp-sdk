#include "backpack/websocket_client.hpp"
#include <openssl/evp.h>
#include <openssl/encoder.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <iostream>
#include <sstream>
#include <cstring>
#include <thread>
#include <chrono>

namespace backpack {

inline std::string base64_encode(const unsigned char* buf, std::size_t len)
{
    std::string out(4 * ((len + 2) / 3), '\0');
    int out_len = EVP_EncodeBlock(
        reinterpret_cast<unsigned char*>(out.data()), buf, static_cast<int>(len));
    out.resize(out_len);
    return out;
}

inline std::string base64_decode(const std::string& in)
{
    std::vector<unsigned char> tmp(in.size());
    int len = EVP_DecodeBlock(tmp.data(),
                              reinterpret_cast<const unsigned char*>(in.data()),
                              static_cast<int>(in.size()));
    if (len < 0)
        throw std::runtime_error("base64 decode error");
    while (len && tmp[len - 1] == 0) --len;
    return std::string(reinterpret_cast<char*>(tmp.data()), len);
}

static std::string ed25519_sign_b64(const std::string& msg, const std::string& secret_b64)
{
    std::string sk_raw = base64_decode(secret_b64);
    if (sk_raw.size() != 64 && sk_raw.size() != 32)
        throw std::runtime_error("invalid ed25519 secret length");

    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr,
        reinterpret_cast<const unsigned char*>(sk_raw.data()),
        sk_raw.size());
    if (!pkey) throw std::runtime_error("EVP_PKEY_new_raw_private_key");

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); throw std::bad_alloc(); }

    if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestSignInit() failed");
    }

    size_t siglen = 0;
    if (EVP_DigestSign(ctx, nullptr, &siglen,
                       reinterpret_cast<const unsigned char*>(msg.data()),
                       msg.size()) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestSign(len) failed");
    }

    std::vector<unsigned char> sig(siglen);
    if (EVP_DigestSign(ctx, sig.data(), &siglen,
                       reinterpret_cast<const unsigned char*>(msg.data()),
                       msg.size()) != 1) {
        EVP_PKEY_free(pkey);
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("EVP_DigestSign(data) failed");
    }

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return base64_encode(sig.data(), siglen);
}

WebSocketClient::WebSocketClient() : m_running(false) {
    m_ws.setOnMessageCallback([this](const ix::WebSocketMessagePtr& msg) {
        if (msg->type == ix::WebSocketMessageType::Message) {
            if (m_message_handler) {
                m_message_handler(msg->str);
            }
        } else if (msg->type == ix::WebSocketMessageType::Error) {
            if (m_fail_handler) {
                m_fail_handler(msg->errorInfo.reason);
            }
        } else if (msg->type == ix::WebSocketMessageType::Open) {
            if (m_open_handler) {
                m_open_handler();
            }
        } else if (msg->type == ix::WebSocketMessageType::Close) {
            if (m_close_handler) {
                m_close_handler();
            }
        }
    });
}

WebSocketClient::~WebSocketClient() {
    close();
}

bool WebSocketClient::connect(const std::string& uri, const std::string& api_key, const std::string& api_secret) {
    if (is_connected()) {
        throw std::runtime_error("Already connected");
    }

    m_uri = uri;
    m_api_key = api_key;
    m_api_secret = api_secret;

    m_ws.setUrl(uri);
    m_ws.enableAutomaticReconnection();
    m_ws.setHandshakeTimeout(CONNECTION_TIMEOUT_MS);

    // Set up authentication headers
    ix::WebSocketHttpHeaders headers;
    headers["X-API-Key"] = api_key;
    std::string signature = ed25519_sign_b64(api_key, api_secret);
    headers["X-API-Sign"] = signature;
    m_ws.setExtraHeaders(headers);

    if (!m_ws.connect(CONNECTION_TIMEOUT_MS).success) {
        throw std::runtime_error("Failed to connect to server");
    }

    m_running = true;
    return true;
}

bool WebSocketClient::send(const std::string& message) {
    if (!is_connected()) {
        throw std::runtime_error("Not connected to server");
    }

    std::lock_guard<std::mutex> lock(m_queue_mutex);
    if (m_message_queue.size() >= MAX_QUEUE_SIZE) {
        throw std::runtime_error("Message queue is full");
    }

    m_message_queue.push(message);
    return send_with_retry(message, MAX_RETRY_ATTEMPTS);
}

void WebSocketClient::process_message_queue() {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    while (!m_message_queue.empty()) {
        auto& message = m_message_queue.front();
        try {
            if (!send_with_retry(message, MAX_RETRY_ATTEMPTS)) {
                if (m_fail_handler) {
                    m_fail_handler("Failed to send message after max retries");
                }
            }
        } catch (const std::exception& e) {
            if (m_fail_handler) {
                m_fail_handler(e.what());
            }
        }
        m_message_queue.pop();
    }
}

bool WebSocketClient::send_with_retry(const std::string& message, int retries) {
    for (int attempt = 0; attempt < retries; ++attempt) {
        if (!is_connected() && !reconnect()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << attempt)));
            continue;
        }

        if (m_ws.send(message).success) {
            return true;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << attempt)));
    }
    return false;
}

bool WebSocketClient::reconnect() {
    if (m_uri.empty() || m_api_key.empty() || m_api_secret.empty()) {
        throw std::runtime_error("Missing connection parameters");
    }

    close();
    return connect(m_uri, m_api_key, m_api_secret);
}

void WebSocketClient::close() {
    m_running = false;
    if (is_connected()) {
        m_ws.close();
    }
}

bool WebSocketClient::is_connected() const {
    return m_ws.getReadyState() == ix::ReadyState::Open;
}

void WebSocketClient::set_message_handler(std::function<void(const std::string&)> handler) {
    m_message_handler = handler;
}

void WebSocketClient::set_open_handler(std::function<void()> handler) {
    m_open_handler = handler;
}

void WebSocketClient::set_close_handler(std::function<void()> handler) {
    m_close_handler = handler;
}

void WebSocketClient::set_fail_handler(std::function<void(const std::string&)> handler) {
    m_fail_handler = handler;
}

} // namespace backpack
