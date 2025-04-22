#pragma once

#include <string>
#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>

#include "types.hpp"
#include "utils.hpp"
#include "websocket_client.hpp"
#include "rest_client.hpp"

namespace backpack {

/**
 * @brief Main client for Backpack Exchange
 * 
 * This class provides a high-level interface to interact with
 * Backpack Exchange via both WebSocket and REST API connections. It handles:
 * - Market data subscriptions via WebSocket
 * - User data subscriptions (orders, trades, balances, positions) via WebSocket
 * - REST API operations for account management, order placement, etc.
 */
class BackpackClient {
public:
    /**
     * @brief Construct a new BackpackClient object
     * 
     * @param websocket_url WebSocket API URL (default: wss://ws.backpack.exchange)
     * @param rest_url REST API URL (default: https://api.backpack.exchange)
     */
    explicit BackpackClient(const std::string& websocket_url = "wss://ws.backpack.exchange",
                           const std::string& rest_url = "https://api.backpack.exchange");
    
    /**
     * @brief Destroy the BackpackClient object
     */
    ~BackpackClient();
    
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
     * @brief Authenticate the connection with API credentials
     * 
     * @return true if authentication successful, false otherwise
     */
    bool authenticate();
    
    // WebSocket subscriptions
    
    /**
     * @brief Subscribe to ticker updates
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param callback Callback function for ticker data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_ticker(const std::string& symbol, std::function<void(const Ticker&)> callback);
    
    /**
     * @brief Subscribe to trade updates
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param callback Callback function for trade data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_trades(const std::string& symbol, std::function<void(const Trade&)> callback);
    
    /**
     * @brief Subscribe to candle updates
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param interval Candle interval
     * @param callback Callback function for candle data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_candles(const std::string& symbol, Channel interval, std::function<void(const Candle&)> callback);
    
    /**
     * @brief Subscribe to order book updates
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param callback Callback function for order book data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_depth(const std::string& symbol, std::function<void(const OrderBook&)> callback);
    
    /**
     * @brief Subscribe to order book snapshots
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param callback Callback function for order book snapshot data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_depth_snapshot(const std::string& symbol, std::function<void(const OrderBook&)> callback);
    
    /**
     * @brief Subscribe to user order updates
     * 
     * @param callback Callback function for order data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_user_orders(std::function<void(const Order&)> callback);
    
    /**
     * @brief Subscribe to user trade updates
     * 
     * @param callback Callback function for trade data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_user_trades(std::function<void(const Trade&)> callback);
    
    /**
     * @brief Subscribe to user position updates
     * 
     * @param callback Callback function for position data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_user_positions(std::function<void(const Position&)> callback);
    
    /**
     * @brief Subscribe to user balance updates
     * 
     * @param callback Callback function for balance data
     * @return true if subscription successful, false otherwise
     */
    bool subscribe_user_balances(std::function<void(const Balance&)> callback);
    
    /**
     * @brief Unsubscribe from a channel
     * 
     * @param channel Channel to unsubscribe from
     * @param symbol Symbol to unsubscribe from (if applicable)
     * @return true if unsubscription successful, false otherwise
     */
    bool unsubscribe(Channel channel, const std::string& symbol = "");
    
    /**
     * @brief Send ping to keep connection alive
     */
    void ping();
    
    // REST API endpoints
    
    /**
     * @brief Get server time
     * 
     * @return Server time in milliseconds since epoch
     */
    int64_t get_server_time();
    
    /**
     * @brief Get exchange information
     * 
     * @return Exchange information
     */
    ExchangeInfo get_exchange_info();
    
    /**
     * @brief Get ticker for a symbol
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @return Ticker information
     */
    Ticker get_ticker(const std::string& symbol);
    
    /**
     * @brief Get tickers for all symbols
     * 
     * @return Map of symbols to ticker information
     */
    std::map<std::string, Ticker> get_all_tickers();
    
    /**
     * @brief Get order book for a symbol
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param limit Maximum number of price levels to return (default: 100, max: 500)
     * @return Order book
     */
    OrderBook get_order_book(const std::string& symbol, int limit = 100);
    
    /**
     * @brief Get recent trades for a symbol
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param limit Maximum number of trades to return (default: 100, max: 500)
     * @return Vector of trades
     */
    std::vector<Trade> get_recent_trades(const std::string& symbol, int limit = 100);
    
    /**
     * @brief Get historical trades for a symbol
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param limit Maximum number of trades to return (default: 100, max: 1000)
     * @param from_id Return trades after this ID (optional)
     * @return Vector of trades
     */
    std::vector<Trade> get_historical_trades(const std::string& symbol, int limit = 100, const std::string& from_id = "");
    
    /**
     * @brief Get candlestick data for a symbol
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param interval Candle interval
     * @param limit Maximum number of candles to return (default: 100, max: 1000)
     * @param start_time Start time in milliseconds since epoch (optional)
     * @param end_time End time in milliseconds since epoch (optional)
     * @return Vector of candles
     */
    std::vector<Candle> get_candles(const std::string& symbol, Channel interval, 
                                    int limit = 100, int64_t start_time = 0, int64_t end_time = 0);
    
    /**
     * @brief Create a new order
     * 
     * @param order Order to create
     * @return Created order
     */
    Order create_order(const OrderRequest& order);
    
    /**
     * @brief Test order creation without actually placing it
     * 
     * @param order Order to test
     * @return true if order would be valid, false otherwise
     */
    bool test_order(const OrderRequest& order);
    
    /**
     * @brief Cancel an order
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param order_id Order ID to cancel
     * @return true if cancellation successful, false otherwise
     */
    bool cancel_order(const std::string& symbol, const std::string& order_id);
    
    /**
     * @brief Cancel an order by client order ID
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param client_order_id Client order ID to cancel
     * @return true if cancellation successful, false otherwise
     */
    bool cancel_order_by_client_id(const std::string& symbol, const std::string& client_order_id);
    
    /**
     * @brief Cancel all open orders
     * 
     * @param symbol Trading pair to cancel orders for (optional)
     * @return Number of orders cancelled
     */
    int cancel_all_orders(const std::string& symbol = "");
    
    /**
     * @brief Get order status
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param order_id Order ID to query
     * @return Order information
     */
    Order get_order(const std::string& symbol, const std::string& order_id);
    
    /**
     * @brief Get order status by client order ID
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param client_order_id Client order ID to query
     * @return Order information
     */
    Order get_order_by_client_id(const std::string& symbol, const std::string& client_order_id);
    
    /**
     * @brief Get all open orders
     * 
     * @param symbol Trading pair to filter by (optional)
     * @return Vector of open orders
     */
    std::vector<Order> get_open_orders(const std::string& symbol = "");
    
    /**
     * @brief Get all orders
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param limit Maximum number of orders to return (default: 100, max: 1000)
     * @param from_id Return orders after this ID (optional)
     * @return Vector of orders
     */
    std::vector<Order> get_all_orders(const std::string& symbol, int limit = 100, 
                                     const std::string& from_id = "");
    
    /**
     * @brief Get account information
     * 
     * @return Account information
     */
    Account get_account();
    
    /**
     * @brief Get account balances
     * 
     * @return Vector of balances
     */
    std::vector<Balance> get_balances();
    
    /**
     * @brief Get account trades
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param limit Maximum number of trades to return (default: 100, max: 1000)
     * @param from_id Return trades after this ID (optional)
     * @return Vector of trades
     */
    std::vector<Trade> get_account_trades(const std::string& symbol, int limit = 100,
                                         const std::string& from_id = "");

private:
    template<typename T>
    bool subscribe_to_channel(Channel channel, const std::string& symbol, std::function<void(const T&)> callback) {
        // Implementation in source file
        return false;
    }

    std::unique_ptr<WebSocketClient> ws_client_;
    std::unique_ptr<RestClient> rest_client_;
    std::string websocket_url_;
    std::string rest_url_;
    std::string api_key_;
    std::string api_secret_;
    bool connected_ = false;
    bool authenticated_ = false;
    std::mutex mutex_;
    
    std::map<std::string, std::function<void(const nlohmann::json&)>> message_handlers_;
};

} // namespace backpack
