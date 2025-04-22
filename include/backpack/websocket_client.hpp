#ifndef BACKPACK_WEBSOCKET_CLIENT_HPP
#define BACKPACK_WEBSOCKET_CLIENT_HPP

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <openssl/evp.h>
#include <openssl/encoder.h>
#include <openssl/core_names.h>
#include <openssl/err.h>
#include <vector>
#include <stdexcept>

namespace backpack {

// Base64 helpers
inline std::string base64_encode(const unsigned char* buf, std::size_t len);
inline std::string base64_decode(const std::string& in);

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    
    bool connect(const std::string& uri);
    void send(const std::string& message);
    bool is_connected() const;
    
    void set_message_handler(std::function<void(const std::string&)> handler);
    void set_open_handler(std::function<void()> handler);
    void set_close_handler(std::function<void()> handler);
    void set_fail_handler(std::function<void(const std::string&)> handler);

private:
    std::string ed25519_sign_b64(const std::string& msg, const std::string& secret_b64);
    void async_read();

    net::io_context m_ioc;
    ssl::context m_ssl_ctx;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> m_ws;
    tcp::resolver m_resolver;
    beast::flat_buffer m_buffer;
    std::shared_ptr<std::thread> m_thread;

    std::function<void(const std::string&)> m_message_handler;
    std::function<void()> m_open_handler;
    std::function<void()> m_close_handler;
    std::function<void(const std::string&)> m_fail_handler;
    bool m_connected{false};
};

} // namespace backpack

#endif // BACKPACK_WEBSOCKET_CLIENT_HPP
