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

WebSocketClient::WebSocketClient() 
    : m_ssl_ctx(ssl::context::tlsv12_client)
    , m_ws(m_ioc, m_ssl_ctx)
    , m_resolver(m_ioc)
    , m_running(false) {
    
    // Configure SSL context
    m_ssl_ctx.set_verify_mode(ssl::verify_peer);
    m_ssl_ctx.set_default_verify_paths();

    // Set up default handlers
    m_message_handler = [](const std::string& msg) {
        std::cout << "Received: " << msg << std::endl;
    };
    
    m_open_handler = []() {
        std::cout << "WebSocket connected" << std::endl;
    };
    
    m_close_handler = []() {
        std::cout << "WebSocket disconnected" << std::endl;
    };
    
    m_fail_handler = [](const std::string& error) {
        std::cerr << "WebSocket error: " << error << std::endl;
    };
}

WebSocketClient::~WebSocketClient() {
    cleanup();
}

bool WebSocketClient::connect(const std::string& uri) {
    if (is_connected()) {
        return true;
    }

    // Parse the URI
    std::string host;
    std::string port = "443";  // Default to HTTPS port
    std::string target = "/";

    size_t scheme_end = uri.find("://");
    if (scheme_end != std::string::npos) {
        size_t host_start = scheme_end + 3;
        size_t host_end = uri.find(':', host_start);
        if (host_end == std::string::npos) {
            host_end = uri.find('/', host_start);
            if (host_end == std::string::npos) {
                host = uri.substr(host_start);
            } else {
                host = uri.substr(host_start, host_end - host_start);
                target = uri.substr(host_end);
            }
        } else {
            host = uri.substr(host_start, host_end - host_start);
            size_t port_end = uri.find('/', host_end + 1);
            if (port_end == std::string::npos) {
                port = uri.substr(host_end + 1);
            } else {
                port = uri.substr(host_end + 1, port_end - (host_end + 1));
                target = uri.substr(port_end);
            }
        }
    }

    try {
        // Look up the domain name
        auto const results = m_resolver.resolve(host, port);
        if (results.empty()) {
            if (m_fail_handler) m_fail_handler("DNS resolution failed");
            return false;
        }

        // Set up the SSL stream
        if (!SSL_set_tlsext_host_name(m_ws.next_layer().native_handle(), host.c_str())) {
            if (m_fail_handler) m_fail_handler("SSL hostname setup failed");
            return false;
        }

        // Make the connection on the IP address we get from a lookup
        beast::get_lowest_layer(m_ws).connect(results);

        // Perform the SSL handshake
        m_ws.next_layer().handshake(ssl::stream_base::client);

        // Set the SNI hostname
        m_ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req) {
                req.set(http::field::user_agent, "Backpack C++ SDK");
            }));

        // Perform the websocket handshake
        m_ws.handshake(host, target);

        m_running = true;
        m_connected = true;

        if (m_open_handler) {
            m_open_handler();
        }

        // Start the io_context thread
        m_thread = std::make_shared<std::thread>([this]() {
            try {
                // Start the first async read
                async_read();
                
                // Run the io_context
                m_ioc.run();
            } catch (const std::exception& e) {
                if (m_fail_handler) {
                    m_fail_handler(std::string("IO context error: ") + e.what());
                }
            }
        });

        // Start the heartbeat thread
        m_heartbeat_thread = std::make_shared<std::thread>([this]() {
            try {
                start_heartbeat();
            } catch (const std::exception& e) {
                if (m_fail_handler) {
                    m_fail_handler(std::string("Heartbeat error: ") + e.what());
                }
            }
        });

        return true;
    } catch (const std::exception& e) {
        if (m_fail_handler) {
            m_fail_handler(std::string("Connection error: ") + e.what());
        }
        return false;
    }
}

void WebSocketClient::send(const std::string& message) {
    if (!is_connected()) {
        throw WebSocketError(WebSocketError::Type::CONNECTION_ERROR, "Not connected");
    }

    // Post the write operation to the io_context
    net::post(m_ioc, [this, message]() {
        try {
            m_ws.write(net::buffer(message));
        } catch (const std::exception& e) {
            if (m_fail_handler) {
                m_fail_handler(std::string("Write error: ") + e.what());
            }
        }
    });
}

bool WebSocketClient::send_with_retry(const std::string& message, int max_retries) {
    for (int i = 0; i < max_retries; ++i) {
        try {
            send(message);
            return true;
        } catch (const WebSocketError&) {
            if (i == max_retries - 1) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << i)));
        }
    }
    return false;
}

bool WebSocketClient::is_connected() const {
    return m_connected;
}

void WebSocketClient::set_message_handler(std::function<void(const std::string&)> handler) {
    m_message_handler = std::move(handler);
}

void WebSocketClient::set_open_handler(std::function<void()> handler) {
    m_open_handler = std::move(handler);
}

void WebSocketClient::set_close_handler(std::function<void()> handler) {
    m_close_handler = std::move(handler);
}

void WebSocketClient::set_fail_handler(std::function<void(const std::string&)> handler) {
    m_fail_handler = std::move(handler);
}

void WebSocketClient::async_read() {
    // Read a message into our buffer
    m_ws.async_read(
        m_buffer,
        [this](beast::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                handle_disconnect();
                return;
            }

            // Call the message handler with the received data
            if (m_message_handler) {
                m_message_handler(beast::buffers_to_string(m_buffer.data()));
            }

            // Clear the buffer
            m_buffer.consume(m_buffer.size());

            // Queue up another read
            if (m_running) {
                async_read();
            }
        });
}

void WebSocketClient::handle_disconnect() {
    m_connected = false;
    if (m_close_handler) {
        m_close_handler();
    }
}

void WebSocketClient::start_heartbeat() {
    while (m_running) {
        if (is_connected()) {
            try {
                send("{\"method\":\"PING\"}");
            } catch (const WebSocketError&) {
                // Ignore ping errors
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(HEARTBEAT_INTERVAL));
    }
}

void WebSocketClient::cleanup() {
    m_running = false;
    
    if (is_connected()) {
        try {
            // Post the close operation to the io_context
            net::post(m_ioc, [this]() {
                try {
                    m_ws.close(websocket::close_code::normal);
                } catch (...) {
                    // Ignore errors during cleanup
                }
            });
        } catch (...) {
            // Ignore errors during cleanup
        }
    }
    
    // Stop the io_context
    m_ioc.stop();
    
    if (m_thread && m_thread->joinable()) {
        m_thread->join();
    }
    
    if (m_heartbeat_thread && m_heartbeat_thread->joinable()) {
        m_heartbeat_thread->join();
    }
    
    m_connected = false;
}

} // namespace backpack
