#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "types.hpp"
#include "utils.hpp"

namespace backpack {

using json = nlohmann::json;

/**
 * @brief HTTP method type
 */
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE
};

/**
 * @brief REST API client for Backpack Exchange
 * 
 * This class provides a wrapper around the Backpack Exchange REST API,
 * handling authentication, request signing, and HTTP communication.
 */
class RestClient {
public:
    /**
     * @brief Construct a new RestClient object
     * 
     * @param base_url Base API URL (default: https://api.backpack.exchange)
     */
    explicit RestClient(const std::string& base_url = "https://api.backpack.exchange");
    
    /**
     * @brief Destroy the RestClient object
     */
    ~RestClient();
    
    /**
     * @brief Set API credentials
     * 
     * @param api_key API key
     * @param api_secret API secret
     */
    void set_credentials(const std::string& api_key, const std::string& api_secret);
    
    /**
     * @brief Check if credentials are set
     * 
     * @return true if credentials are set, false otherwise
     */
    bool has_credentials() const;
    
    // Public API Endpoints
    
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
    
    // Authenticated API Endpoints
    
    /**
     * @brief Create a new order
     * 
     * @param order Order to create
     * @return Created order
     */
    Order create_order(const OrderRequest& order);
    
    /**
     * @brief Test creating an order without actually placing it
     * 
     * @param order Order to test
     * @return true if order would be valid, false otherwise
     */
    bool test_order(const OrderRequest& order);
    
    /**
     * @brief Cancel an order
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param order_id Order ID
     * @return true if cancellation successful, false otherwise
     */
    bool cancel_order(const std::string& symbol, const std::string& order_id);
    
    /**
     * @brief Cancel an order using client order ID
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param client_order_id Client order ID
     * @return true if cancellation successful, false otherwise
     */
    bool cancel_order_by_client_id(const std::string& symbol, const std::string& client_order_id);
    
    /**
     * @brief Cancel all open orders for a symbol
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC"), if empty cancels for all symbols
     * @return Number of orders canceled
     */
    int cancel_all_orders(const std::string& symbol = "");
    
    /**
     * @brief Get an order by ID
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param order_id Order ID
     * @return Order information
     */
    Order get_order(const std::string& symbol, const std::string& order_id);
    
    /**
     * @brief Get an order by client order ID
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param client_order_id Client order ID
     * @return Order information
     */
    Order get_order_by_client_id(const std::string& symbol, const std::string& client_order_id);
    
    /**
     * @brief Get all open orders
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC"), if empty returns orders for all symbols
     * @return Vector of orders
     */
    std::vector<Order> get_open_orders(const std::string& symbol = "");
    
    /**
     * @brief Get all orders (open and closed)
     * 
     * @param symbol Trading pair (e.g., "SOL-USDC")
     * @param limit Maximum number of orders to return (default: 100, max: 500)
     * @param start_time Start time in milliseconds since epoch (optional)
     * @param end_time End time in milliseconds since epoch (optional)
     * @return Vector of orders
     */
    std::vector<Order> get_all_orders(const std::string& symbol, int limit = 100, 
                                     int64_t start_time = 0, int64_t end_time = 0);
    
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
     * @param limit Maximum number of trades to return (default: 100, max: 500)
     * @param start_time Start time in milliseconds since epoch (optional)
     * @param end_time End time in milliseconds since epoch (optional)
     * @return Vector of trades
     */
    std::vector<Trade> get_account_trades(const std::string& symbol, int limit = 100,
                                         int64_t start_time = 0, int64_t end_time = 0);
    
private:
    std::string base_url_;
    Credentials credentials_;
    CURL* curl_;
    
    /**
     * @brief Send a request to the API
     * 
     * @param endpoint API endpoint
     * @param method HTTP method
     * @param params Query parameters
     * @param body Request body (for POST/PUT requests)
     * @param auth_required Whether the request requires authentication
     * @return API response as JSON
     */
    json send_request(const std::string& endpoint, HttpMethod method, 
                     const std::map<std::string, std::string>& params = {},
                     const std::string& body = "", bool auth_required = false);
    
    /**
     * @brief Sign a request
     * 
     * @param method HTTP method
     * @param endpoint API endpoint
     * @param timestamp Request timestamp
     * @param params Query parameters
     * @param body Request body
     * @return Request signature
     */
    std::string sign_request(HttpMethod method, const std::string& endpoint, int64_t timestamp,
                            const std::map<std::string, std::string>& params, const std::string& body);
    
    /**
     * @brief Convert HTTP method to string
     * 
     * @param method HTTP method
     * @return Method as string
     */
    std::string http_method_to_string(HttpMethod method);
    
    /**
     * @brief Callback function for curl to write response data
     */
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data);
};

} // namespace backpack 