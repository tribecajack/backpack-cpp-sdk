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

namespace backpack {

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
using tcp = boost::asio::ip::tcp;

class WebSocketClient {
public:
    WebSocketClient() 
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
    
    ~WebSocketClient() {
        if (m_thread) {
            m_ioc.stop();
            m_thread->join();
        }
    }
    
    bool connect(const std::string& uri) {
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
            
            return true;
        } catch (std::exception& e) {
            return false;
        }
    }
    
    void send(const std::string& message) {
        try {
            m_ws.write(net::buffer(message));
        } catch (std::exception& e) {
            if (m_fail_handler) {
                m_fail_handler("Failed to send message: " + std::string(e.what()));
            }
        }
    }
    
    void set_message_handler(std::function<void(const std::string&)> handler) {
        m_message_handler = handler;
        
        // Start reading messages
        async_read();
    }
    
    void set_open_handler(std::function<void()> handler) {
        m_open_handler = handler;
    }
    
    void set_close_handler(std::function<void()> handler) {
        m_close_handler = handler;
    }
    
    void set_fail_handler(std::function<void(const std::string&)> handler) {
        m_fail_handler = handler;
    }

private:
// Boost.Beast implementation ― kept from *main*

    void async_read()
    {
        m_ws.async_read(
            m_buffer,
            [this](beast::error_code ec, std::size_t /*bytes_transferred*/)
            {
                if (!ec)
                {
                    if (m_message_handler)
                        m_message_handler(beast::buffers_to_string(m_buffer.data()));

                    m_buffer.consume(m_buffer.size());
                    async_read();
                }
                else
                {
                    if (m_fail_handler)
                        m_fail_handler("Read failed: " + ec.message());
                }
            });
    }

    net::io_context                                         m_ioc;
    ssl::context                                            m_ssl_ctx;
    websocket::stream<beast::ssl_stream<beast::tcp_stream>> m_ws;
    tcp::resolver                                           m_resolver;
    beast::flat_buffer                                      m_buffer;
    std::shared_ptr<std::thread>                            m_thread;

    std::function<void(const std::string&)> m_message_handler;
    std::function<void()>                   m_open_handler;
    std::function<void()>                   m_close_handler;
    std::function<void(const std::string&)> m_fail_handler;

// OPTIONAL: secure_rest branch additions
// Commented‑out for now; add missing types/headers

#if 0
    std::string                      base_url_;
    Credentials                      credentials_;
    WebsocketClient                  client_;
    WebsocketConnectionHdl           connection_handle_;
    std::thread                      client_thread_;
    std::atomic<bool>                connected_{false};
    std::atomic<bool>                authenticated_{false};
    std::atomic<bool>                running_{false};
    std::mutex                       mutex_;

    std::map<std::string, MessageCallback> callbacks_;
    MessageCallback                     general_callback_;

    std::queue<std::string>             message_queue_;
    std::mutex                          queue_mutex_;
    std::condition_variable             queue_condition_;
    std::thread                         send_thread_;

    WebsocketTlsContext on_tls_init();

    // WebSocket event handlers
    void on_open(WebsocketConnectionHdl hdl);
    void on_close(WebsocketConnectionHdl hdl);
    void on_message(WebsocketConnectionHdl hdl,
                    WebsocketClient::message_ptr msg);
    void on_error(WebsocketConnectionHdl hdl,
                  const websocketpp::lib::error_code& ec);

    // Helper methods
    void         run_client();
    void         process_message_queue();
    bool         send_message(const std::string& message);
    std::string  get_callback_key(Channel channel,
                                  const std::string& symbol);
    void         handle_message(const json& message);
    std::string  generate_signature(const std::string& message,
                                    const std::string& secret);
#endif
};


} // namespace backpack

#endif // BACKPACK_WEBSOCKET_CLIENT_HPP
