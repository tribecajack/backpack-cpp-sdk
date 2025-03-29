#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <queue>
#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include <nlohmann/json.hpp>

#include "types.hpp"
#include "utils.hpp"

namespace backpack {

// Define WebSocket connection types
using WebsocketClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using WebsocketConnectionHdl = websocketpp::connection_hdl;
using WebsocketTlsContext = websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>;

/**
 * @brief WebSocket client for Backpack Exchange
 * 
 * This class handles WebSocket connections to Backpack Exchange,
 * including authentication, subscribing to channels, and message processing.
 */
class BackpackWebSocketClient {
public:
    /**
     * @brief Construct a new BackpackWebSocketClient object
     * 
     * @param base_url Base WebSocket URL (default: wss://ws.backpack.exchange)
     */
    explicit BackpackWebSocketClient(const std::string& base_url = "wss://ws.backpack.exchange");
    
    /**
     * @brief Destroy the BackpackWebSocketClient object
     */
    ~BackpackWebSocketClient();
    
    /**
     * @brief Set API credentials for authenticated endpoints
     * 
     * @param api_key API key
     * @param api_secret API secret
     */
    void set_credentials(const std::string& api_key, const std::string& api_secret);
    
    /**
     * @brief Connect to the WebSocket server
     * 
     * @return true if connection successful, false otherwise
     */
    bool connect();
    
    /**
     * @brief Disconnect from the WebSocket server
     */
    void disconnect();
    
    /**
     * @brief Check if connected to the WebSocket server
     * 
     * @return true if connected, false otherwise
     */
    bool is_connected() const;
    
    /**
     * @brief Authenticate the WebSocket connection
     * 
     * @return true if authentication successful, false otherwise
     */
    bool authenticate();
    
    /**
     * @brief Subscribe to a channel
     * 
     * @param channel Channel to subscribe to
     * @param symbol Symbol to subscribe for (e.g., "SOL-USDC")
     * @return true if subscription request sent, false otherwise
     */
    bool subscribe(Channel channel, const std::string& symbol);
    
    /**
     * @brief Unsubscribe from a channel
     * 
     * @param channel Channel to unsubscribe from
     * @param symbol Symbol to unsubscribe from
     * @return true if unsubscription request sent, false otherwise
     */
    bool unsubscribe(Channel channel, const std::string& symbol);
    
    /**
     * @brief Register callback for a specific channel and symbol
     * 
     * @param channel Channel to receive messages for
     * @param symbol Symbol to receive messages for (can be empty for user channels)
     * @param callback Callback function to handle messages
     */
    void register_callback(Channel channel, const std::string& symbol, MessageCallback callback);
    
    /**
     * @brief Register a general message callback for all messages
     * 
     * @param callback Callback function to handle all messages
     */
    void register_general_callback(MessageCallback callback);
    
    /**
     * @brief Send ping to keep connection alive
     */
    void ping();

private:
    std::string base_url_;
    Credentials credentials_;
    WebsocketClient client_;
    WebsocketConnectionHdl connection_handle_;
    std::thread client_thread_;
    std::atomic<bool> connected_;
    std::atomic<bool> authenticated_;
    std::atomic<bool> running_;
    std::mutex mutex_;
    
    // Callbacks for different channels and symbols
    std::map<std::string, MessageCallback> callbacks_;
    MessageCallback general_callback_;
    
    // Message queue for sending
    std::queue<std::string> message_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_condition_;
    std::thread send_thread_;
    
    // TLS initialization
    WebsocketTlsContext on_tls_init();
    
    // WebSocket event handlers
    void on_open(WebsocketConnectionHdl hdl);
    void on_close(WebsocketConnectionHdl hdl);
    void on_message(WebsocketConnectionHdl hdl, WebsocketClient::message_ptr msg);
    void on_error(WebsocketConnectionHdl hdl, const websocketpp::lib::error_code& ec);
    
    // Helper methods
    void run_client();
    void process_message_queue();
    bool send_message(const std::string& message);
    std::string get_callback_key(Channel channel, const std::string& symbol);
    void handle_message(const json& message);
};

} // namespace backpack
