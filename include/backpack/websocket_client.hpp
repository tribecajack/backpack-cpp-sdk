#ifndef BACKPACK_WEBSOCKET_CLIENT_HPP
#define BACKPACK_WEBSOCKET_CLIENT_HPP

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <openssl/evp.h>
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

// Custom error types
class WebSocketError : public std::runtime_error {
public:
    enum class Type {
        CONNECTION_ERROR,
        AUTHENTICATION_ERROR,
        SUBSCRIPTION_ERROR,
        MESSAGE_ERROR,
        NETWORK_ERROR
    };
    
    WebSocketError(Type type, const std::string& message) 
        : std::runtime_error(message), type_(type) {}
    
    Type type() const { return type_; }
    
private:
    Type type_;
};

class WebSocketClient {
public:
    WebSocketClient();
    ~WebSocketClient();
    
    bool connect(const std::string& uri);
    void send(const std::string& message);
    bool send_with_retry(const std::string& message, int max_retries = 3);
    bool is_connected() const;
    
    void set_message_handler(std::function<void(const std::string&)> handler);
    void set_open_handler(std::function<void()> handler);
    void set_close_handler(std::function<void()> handler);
    void set_fail_handler(std::function<void(const std::string&)> handler);

private:
    std::string ed25519_sign_b64(const std::string& msg, const std::string& secret_b64);
    void async_read();
    void handle_disconnect();
    void start_heartbeat();
    void cleanup();
    void process_message_queue();

    net::io_context m_ioc;
    ssl::context m_ssl_ctx;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> m_ws;
    tcp::resolver m_resolver;
    beast::flat_buffer m_buffer;
    std::shared_ptr<std::thread> m_thread;
    std::shared_ptr<std::thread> m_heartbeat_thread;
    
    std::queue<std::string> m_message_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;
    
    std::function<void(const std::string&)> m_message_handler;
    std::function<void()> m_open_handler;
    std::function<void()> m_close_handler;
    std::function<void(const std::string&)> m_fail_handler;
    
    std::string m_last_uri;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_running{true};
    
    static constexpr int HEARTBEAT_INTERVAL = 30; // seconds
    static constexpr int MAX_RECONNECT_ATTEMPTS = 5;
    static constexpr int QUEUE_MAX_SIZE = 1000;
};

} // namespace backpack

#endif // BACKPACK_WEBSOCKET_CLIENT_HPP
