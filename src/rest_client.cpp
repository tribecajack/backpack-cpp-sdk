#include "backpack/rest_client.hpp"
#include <iostream>
#include <sstream>
#include <curl/curl.h>

namespace backpack {

RestClient::RestClient(const std::string& base_url)
    : base_url_(base_url) {
    
    // Initialize curl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl_ = curl_easy_init();
    
    if (!curl_) {
        throw std::runtime_error("Failed to initialize curl");
    }
}

RestClient::~RestClient() {
    // Clean up curl
    if (curl_) {
        curl_easy_cleanup(curl_);
        curl_ = nullptr;
    }
    curl_global_cleanup();
}

void RestClient::set_credentials(const std::string& api_key, const std::string& api_secret) {
    credentials_.api_key = api_key;
    credentials_.api_secret = api_secret;
}

bool RestClient::has_credentials() const {
    return credentials_.is_valid();
}

int64_t RestClient::get_server_time() {
    json response = send_request("/api/v1/time", HttpMethod::GET);
    return response["serverTime"].get<int64_t>();
}

ExchangeInfo RestClient::get_exchange_info() {
    json response = send_request("/api/v1/exchangeInfo", HttpMethod::GET);
    return ExchangeInfo::from_json(response);
}

Ticker RestClient::get_ticker(const std::string& symbol) {
    std::map<std::string, std::string> params = {{"symbol", symbol}};
    json response = send_request("/api/v1/ticker", HttpMethod::GET, params);
    return Ticker::from_json(response);
}

std::map<std::string, Ticker> RestClient::get_all_tickers() {
    json response = send_request("/api/v1/tickers", HttpMethod::GET);
    
    std::map<std::string, Ticker> tickers;
    for (const auto& ticker_json : response) {
        Ticker ticker = Ticker::from_json(ticker_json);
        tickers[ticker.symbol] = ticker;
    }
    
    return tickers;
}

OrderBook RestClient::get_order_book(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"limit", std::to_string(limit)}
    };
    
    json response = send_request("/api/v1/depth", HttpMethod::GET, params);
    return OrderBook::from_json(response);
}

std::vector<Trade> RestClient::get_recent_trades(const std::string& symbol, int limit) {
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"limit", std::to_string(limit)}
    };
    
    json response = send_request("/api/v1/trades", HttpMethod::GET, params);
    
    std::vector<Trade> trades;
    for (const auto& trade_json : response) {
        trades.push_back(Trade::from_json(trade_json));
    }
    
    return trades;
}

std::vector<Trade> RestClient::get_historical_trades(const std::string& symbol, int limit, const std::string& from_id) {
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"limit", std::to_string(limit)}
    };
    
    if (!from_id.empty()) {
        params["fromId"] = from_id;
    }
    
    json response = send_request("/api/v1/historicalTrades", HttpMethod::GET, params, "", true);
    
    std::vector<Trade> trades;
    for (const auto& trade_json : response) {
        trades.push_back(Trade::from_json(trade_json));
    }
    
    return trades;
}

std::vector<Candle> RestClient::get_candles(const std::string& symbol, Channel interval, 
                                           int limit, int64_t start_time, int64_t end_time) {
    std::string interval_str;
    
    // Convert channel to interval string
    switch (interval) {
        case Channel::CANDLES_1M: interval_str = "1m"; break;
        case Channel::CANDLES_5M: interval_str = "5m"; break;
        case Channel::CANDLES_15M: interval_str = "15m"; break;
        case Channel::CANDLES_1H: interval_str = "1h"; break;
        case Channel::CANDLES_4H: interval_str = "4h"; break;
        case Channel::CANDLES_1D: interval_str = "1d"; break;
        default:
            throw std::invalid_argument("Invalid candle interval");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"interval", interval_str},
        {"limit", std::to_string(limit)}
    };
    
    if (start_time > 0) {
        params["startTime"] = std::to_string(start_time);
    }
    
    if (end_time > 0) {
        params["endTime"] = std::to_string(end_time);
    }
    
    json response = send_request("/api/v1/klines", HttpMethod::GET, params);
    
    std::vector<Candle> candles;
    for (const auto& candle_json : response) {
        // API returns an array like [timestamp, open, high, low, close, volume]
        Candle candle;
        candle.symbol = symbol;
        candle.timestamp = std::to_string(candle_json[0].get<int64_t>());
        candle.open = std::stod(candle_json[1].get<std::string>());
        candle.high = std::stod(candle_json[2].get<std::string>());
        candle.low = std::stod(candle_json[3].get<std::string>());
        candle.close = std::stod(candle_json[4].get<std::string>());
        candle.volume = std::stod(candle_json[5].get<std::string>());
        candles.push_back(candle);
    }
    
    return candles;
}

Order RestClient::create_order(const OrderRequest& order_request) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::string body = order_request.to_json().dump();
    json response = send_request("/api/v1/order", HttpMethod::POST, {}, body, true);
    
    return Order::from_json(response);
}

bool RestClient::test_order(const OrderRequest& order_request) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::string body = order_request.to_json().dump();
    try {
        send_request("/api/v1/order/test", HttpMethod::POST, {}, body, true);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Order test failed: " << e.what() << std::endl;
        return false;
    }
}

bool RestClient::cancel_order(const std::string& symbol, const std::string& order_id) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"orderId", order_id}
    };
    
    try {
        send_request("/api/v1/order", HttpMethod::DELETE, params, "", true);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Cancel order failed: " << e.what() << std::endl;
        return false;
    }
}

bool RestClient::cancel_order_by_client_id(const std::string& symbol, const std::string& client_order_id) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"clientOrderId", client_order_id}
    };
    
    try {
        send_request("/api/v1/order", HttpMethod::DELETE, params, "", true);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Cancel order by client ID failed: " << e.what() << std::endl;
        return false;
    }
}

int RestClient::cancel_all_orders(const std::string& symbol) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    
    json response = send_request("/api/v1/openOrders", HttpMethod::DELETE, params, "", true);
    return response.value("count", 0);
}

Order RestClient::get_order(const std::string& symbol, const std::string& order_id) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"orderId", order_id}
    };
    
    json response = send_request("/api/v1/order", HttpMethod::GET, params, "", true);
    return Order::from_json(response);
}

Order RestClient::get_order_by_client_id(const std::string& symbol, const std::string& client_order_id) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"clientOrderId", client_order_id}
    };
    
    json response = send_request("/api/v1/order", HttpMethod::GET, params, "", true);
    return Order::from_json(response);
}

std::vector<Order> RestClient::get_open_orders(const std::string& symbol) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params;
    if (!symbol.empty()) {
        params["symbol"] = symbol;
    }
    
    json response = send_request("/api/v1/openOrders", HttpMethod::GET, params, "", true);
    
    std::vector<Order> orders;
    for (const auto& order_json : response) {
        orders.push_back(Order::from_json(order_json));
    }
    
    return orders;
}

std::vector<Order> RestClient::get_all_orders(const std::string& symbol, int limit, 
                                            int64_t start_time, int64_t end_time) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"limit", std::to_string(limit)}
    };
    
    if (start_time > 0) {
        params["startTime"] = std::to_string(start_time);
    }
    
    if (end_time > 0) {
        params["endTime"] = std::to_string(end_time);
    }
    
    json response = send_request("/api/v1/allOrders", HttpMethod::GET, params, "", true);
    
    std::vector<Order> orders;
    for (const auto& order_json : response) {
        orders.push_back(Order::from_json(order_json));
    }
    
    return orders;
}

Account RestClient::get_account() {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    json response = send_request("/api/v1/account", HttpMethod::GET, {}, "", true);
    return Account::from_json(response);
}

std::vector<Balance> RestClient::get_balances() {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    json response = send_request("/api/v1/balances", HttpMethod::GET, {}, "", true);
    
    std::vector<Balance> balances;
    for (const auto& balance_json : response) {
        balances.push_back(Balance::from_json(balance_json));
    }
    
    return balances;
}

std::vector<Trade> RestClient::get_account_trades(const std::string& symbol, int limit,
                                                int64_t start_time, int64_t end_time) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"limit", std::to_string(limit)}
    };
    
    if (start_time > 0) {
        params["startTime"] = std::to_string(start_time);
    }
    
    if (end_time > 0) {
        params["endTime"] = std::to_string(end_time);
    }
    
    json response = send_request("/api/v1/myTrades", HttpMethod::GET, params, "", true);
    
    std::vector<Trade> trades;
    for (const auto& trade_json : response) {
        trades.push_back(Trade::from_json(trade_json));
    }
    
    return trades;
}

json RestClient::send_request(const std::string& endpoint, HttpMethod method, 
                             const std::map<std::string, std::string>& params,
                             const std::string& body, bool auth_required) {
    if (auth_required && !has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    // Reset curl handle
    curl_easy_reset(curl_);
    
    // Build query string
    std::string query_string = build_query_string(params);
    
    // Full URL
    std::string url = base_url_ + endpoint;
    if (!query_string.empty() && method != HttpMethod::POST) {
        url += "?" + query_string;
    }
    
    // Set URL
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    
    // Set HTTP method
    std::string method_str = http_method_to_string(method);
    if (method != HttpMethod::GET) {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, method_str.c_str());
    }
    
    // Headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");
    
    // Add authentication if required
    if (auth_required) {
        int64_t timestamp = get_current_timestamp_ms();
        std::string signature = sign_request(method, endpoint, timestamp, params, body);
        
        headers = curl_slist_append(headers, ("X-API-Key: " + credentials_.api_key).c_str());
        headers = curl_slist_append(headers, ("X-Timestamp: " + std::to_string(timestamp)).c_str());
        headers = curl_slist_append(headers, ("X-Signature: " + signature).c_str());
    }
    
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    // Set request body for POST
    if (method == HttpMethod::POST && !body.empty()) {
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
    }
    
    // Setup response handling
    std::string response_string;
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_string);
    
    // Perform request
    CURLcode res = curl_easy_perform(curl_);
    
    // Clean up headers
    curl_slist_free_all(headers);
    
    // Check for errors
    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl_easy_perform() failed: ") + curl_easy_strerror(res));
    }
    
    // Parse response
    try {
        json response = json::parse(response_string);
        
        // Check for API error
        if (response.contains("code") && response.contains("msg")) {
            throw std::runtime_error("API error: " + response["msg"].get<std::string>());
        }
        
        return response;
    } catch (const json::parse_error& e) {
        throw std::runtime_error(std::string("Failed to parse response: ") + e.what());
    }
}

std::string RestClient::sign_request(HttpMethod method, const std::string& endpoint, int64_t timestamp,
                                    const std::map<std::string, std::string>& params, const std::string& body) {
    // Build message to sign
    std::string method_str = http_method_to_string(method);
    std::string query_string = build_query_string(params);
    
    std::stringstream message_stream;
    message_stream << method_str;
    message_stream << endpoint;
    
    if (!query_string.empty()) {
        message_stream << "?" << query_string;
    }
    
    message_stream << timestamp;
    
    if (!body.empty()) {
        message_stream << body;
    }
    
    std::string message = message_stream.str();
    
    // Generate signature
    return generate_hmac_sha256(message, credentials_.api_secret);
}

std::string RestClient::http_method_to_string(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        default: return "GET";
    }
}

size_t RestClient::write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    if (data) {
        data->append(ptr, size * nmemb);
        return size * nmemb;
    }
    return 0;
}

} // namespace backpack 