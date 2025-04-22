#include "backpack/websocket_client.hpp"
#include <iostream>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <cstring>

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
    // trim padding that EVP_DecodeBlock fills with zeros
    while (len && tmp[len - 1] == 0) --len;
    return std::string(reinterpret_cast<char*>(tmp.data()), len);
}

std::string WebSocketClient::ed25519_sign_b64(const std::string& msg, const std::string& secret_b64)
{
    /* 1 ─ decode secret key */
    std::string sk_raw = base64_decode(secret_b64);
    if (sk_raw.size() != 32 && sk_raw.size() != 64)
        throw std::runtime_error("invalid Ed25519 secret length");

    /* 2 ─ import key */
    EVP_PKEY* pkey = EVP_PKEY_new_raw_private_key(
        EVP_PKEY_ED25519, nullptr,
        reinterpret_cast<const unsigned char*>(sk_raw.data()),
        sk_raw.size());
    if (!pkey) throw std::runtime_error("EVP_PKEY_new_raw_private_key() failed");

    /* 3 ─ one‑shot sign (Init → Sign(len) → Sign(buf)) */
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) { EVP_PKEY_free(pkey); throw std::bad_alloc(); }

    if (EVP_DigestSignInit(ctx, nullptr, nullptr, nullptr, pkey) != 1)
        throw std::runtime_error("EVP_DigestSignInit() failed");

    size_t siglen = 0;
    if (EVP_DigestSign(ctx, nullptr, &siglen,                         // get len
                       reinterpret_cast<const unsigned char*>(msg.data()),
                       msg.size()) != 1)
        throw std::runtime_error("EVP_DigestSign(len) failed");

    std::vector<unsigned char> sig(siglen);
    if (EVP_DigestSign(ctx, sig.data(), &siglen,                      // get data
                       reinterpret_cast<const unsigned char*>(msg.data()),
                       msg.size()) != 1)
        throw std::runtime_error("EVP_DigestSign(data) failed");

    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return base64_encode(sig.data(), siglen);
}

WebSocketClient::WebSocketClient() 
    : m_ioc()
    , m_ssl_ctx(ssl::context::tlsv12_client)
    , m_ws(net::make_strand(m_ioc), m_ssl_ctx)
    , m_resolver(net::make_strand(m_ioc))
{
    m_ssl_ctx.set_verify_mode(ssl::verify_peer);
    m_ssl_ctx.set_default_verify_paths();
    
    m_thread = std::make_shared<std::thread>([this] {
        m_ioc.run();
    });
}

WebSocketClient::~WebSocketClient() {
    if (m_thread) {
        m_ioc.stop();
        m_thread->join();
    }
}

bool WebSocketClient::connect(const std::string& uri) {
    try {
        // Parse the URI
        std::string host;
        std::string port = "443";
        std::string target = "/";
        
        size_t pos = uri.find("://");
        if (pos != std::string::npos) {
            host = uri.substr(pos + 3);
            pos = host.find('/');
            if (pos != std::string::npos) {
                target = host.substr(pos);
                host = host.substr(0, pos);
            }
            pos = host.find(':');
            if (pos != std::string::npos) {
                port = host.substr(pos + 1);
                host = host.substr(0, pos);
            }
        }
        
        // Resolve the host
        auto const results = m_resolver.resolve(host, port);
        
        // Connect to the IP address
        beast::get_lowest_layer(m_ws).connect(results);
        
        // Perform SSL handshake
        m_ws.next_layer().handshake(ssl::stream_base::client);
        
        // Perform WebSocket handshake
        m_ws.handshake(host, target);
        
        m_connected = true;
        return true;
    } catch (std::exception& e) {
        m_connected = false;
        return false;
    }
}

bool WebSocketClient::is_connected() const {
    return m_connected;
}

void WebSocketClient::send(const std::string& message) {
    try {
        m_ws.write(net::buffer(message));
    } catch (std::exception& e) {
        if (m_fail_handler) {
            m_fail_handler("Failed to send message: " + std::string(e.what()));
        }
    }
}

void WebSocketClient::set_message_handler(std::function<void(const std::string&)> handler) {
    m_message_handler = handler;
    async_read();
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

void WebSocketClient::async_read() {
    m_ws.async_read(
        m_buffer,
        [this](beast::error_code ec, std::size_t /*bytes_transferred*/) {
            if (!ec) {
                if (m_message_handler)
                    m_message_handler(beast::buffers_to_string(m_buffer.data()));
                
                m_buffer.consume(m_buffer.size());
                async_read();
            } else {
                if (m_fail_handler)
                    m_fail_handler("Read failed: " + ec.message());
            }
        });
}

} // namespace backpack
