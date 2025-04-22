#include "backpack/rest_client.hpp"
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm> // for std::sort if needed later for param sorting
#include <stdexcept>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/pem.h> // For potential PEM decoding if needed, raw key is likely sufficient
#include <openssl/hmac.h> // Keep for now? No, remove if not used.
#include <openssl/sha.h> // Keep for now? No, remove if not used.
#include <iomanip> // for hex conversion if needed, base64 is needed now.

namespace backpack {

// Helper function for Base64 Decoding
std::vector<unsigned char> base64_decode(const std::string& encoded_string) {
    BIO *bio, *b64;
    int decode_len = encoded_string.length() * 3 / 4; // Estimate length
    std::vector<unsigned char> buffer(decode_len);

    bio = BIO_new_mem_buf(encoded_string.c_str(), -1);
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // No newlines in input
    int length = BIO_read(bio, buffer.data(), encoded_string.length());
    BIO_free_all(bio);

    if (length < 0) {
        throw std::runtime_error("Failed to decode base64 private key");
    }
    buffer.resize(length); // Adjust buffer size to actual decoded length
    return buffer;
}

// Helper function for Base64 Encoding
std::string base64_encode(const unsigned char* buffer, size_t length) {
    BIO *bio, *b64;
    BUF_MEM *buffer_ptr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // No newlines in output
    BIO_write(bio, buffer, length);
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &buffer_ptr);
    
    std::string encoded_string(buffer_ptr->data, buffer_ptr->length);
    
    BIO_free_all(bio); // Also frees b64
    // BUF_MEM_free(buffer_ptr) is done by BIO_free_all

    return encoded_string;
}

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

void RestClient::set_credentials(const std::string& api_key, const std::string& base64_private_key) {
    credentials_.api_key = api_key;
    credentials_.base64_private_key = base64_private_key;
    // Consider decoding and storing the raw private key here for performance
    // Or decode it once in sign_request and cache it. For now, decode every time.
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

std::vector<Order> RestClient::get_all_orders(const std::string& symbol, int limit, const std::string& from_id) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"limit", std::to_string(limit)}
    };
    
    if (!from_id.empty()) {
        params["fromId"] = from_id;
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

std::vector<Trade> RestClient::get_account_trades(const std::string& symbol, int limit, const std::string& from_id) {
    if (!has_credentials()) {
        throw std::runtime_error("API credentials not set");
    }
    
    std::map<std::string, std::string> params = {
        {"symbol", symbol},
        {"limit", std::to_string(limit)}
    };
    
    if (!from_id.empty()) {
        params["fromId"] = from_id;
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
    std::string url = base_url_ + endpoint;
    
    // Add query parameters
    if (!params.empty()) {
        url += "?";
        bool first = true;
        for (const auto& param : params) {
            if (!first) {
                url += "&";
            }
            url += param.first + "=" + param.second;
            first = false;
        }
    }
    
    // Set up request
    curl_easy_reset(curl_);
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
    
    std::string response_data;
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_data);
    
    // Set up headers
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    if (auth_required) {
        if (!has_credentials()) {
            throw std::runtime_error("API credentials not set");
        }
        
        int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        
        // Use a monotonic clock source if system_clock can go backwards
        // std::chrono::steady_clock could be an alternative if strictly monotonic is required
        // However, Backpack likely expects wall time (UTC) for the timestamp.
        // Ensure server time is synced (e.g., via NTP).
        
        std::string signature = sign_request(method, endpoint, timestamp, params, body);
        
        headers = curl_slist_append(headers, ("X-API-KEY: " + credentials_.api_key).c_str());
        headers = curl_slist_append(headers, ("X-BPX-TS: " + std::to_string(timestamp)).c_str()); // Updated header name
        headers = curl_slist_append(headers, ("X-BPX-SIGNATURE: " + signature).c_str()); // Updated header name
    }
    
    curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
    
    // Set method and body
    switch (method) {
        case HttpMethod::GET:
            break;
        case HttpMethod::POST:
            curl_easy_setopt(curl_, CURLOPT_POST, 1L);
            if (!body.empty()) {
                curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
            }
            break;
        case HttpMethod::PUT:
            curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "PUT");
            if (!body.empty()) {
                curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, body.c_str());
            }
            break;
        case HttpMethod::DELETE:
            curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, "DELETE");
            break;
    }
    
    // Perform request
    CURLcode res = curl_easy_perform(curl_);
    
    // Clean up headers
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    // Check response code
    long response_code;
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &response_code);
    
    if (response_code >= 400) {
        throw std::runtime_error("API request failed with code " + std::to_string(response_code) + 
                               ": " + response_data);
    }
    
    // Parse response
    try {
        return json::parse(response_data);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse API response: " + std::string(e.what()));
    }
}

std::string RestClient::sign_request(HttpMethod method, const std::string& endpoint, int64_t timestamp,
                                   const std::map<std::string, std::string>& params, const std::string& body) {
    // Decode the Base64 private key
    std::vector<unsigned char> raw_private_key;
    try {
        raw_private_key = base64_decode(credentials_.base64_private_key);
        // ED25519 private key should be 32 bytes (or 64 bytes seed+pubkey)
        // Assuming the base64 decodes to the 32-byte private key seed.
         if (raw_private_key.size() != 32 && raw_private_key.size() != 64) {
             // OpenSSL's EVP_PKEY_new_raw_private_key expects 32 bytes for ED25519
             // If it's 64 bytes, it might be the seed+pubkey format. Need clarification on Backpack's exact format.
             // Assuming 32 bytes for now. Adjust if needed based on key format.
             // throw std::runtime_error("Invalid ED25519 private key length after base64 decoding: " + std::to_string(raw_private_key.size()));
             // Allowing 64 for now, as some formats include the public key. EVP might handle this.
         }
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to decode private key: " + std::string(e.what()));
    }

    // Construct the message to sign
    // Reference: https://docs.backpack.exchange/#tag/Authentication/Private-Endpoints
    // "All private requests must contain these headers, which are used to authenticate requests."
    // "The signature is generated by signing the request body concatenated with the timestamp header value (X-BPX-TS)."
    // "For GET and DELETE requests, where there is no request body, only the timestamp header value is signed."
    
    std::string message_to_sign;
    std::string timestamp_str = std::to_string(timestamp);

    switch (method) {
        case HttpMethod::GET:
        case HttpMethod::DELETE:
             // For GET/DELETE, sign only the timestamp string
             message_to_sign = timestamp_str;
            break;
        case HttpMethod::POST:
        case HttpMethod::PUT:
            // For POST/PUT, sign request body + timestamp string
            message_to_sign = body + timestamp_str;
            break;
        default:
            throw std::invalid_argument("Unsupported HTTP method for signing");
    }

    // Load the raw private key using OpenSSL EVP
    EVP_PKEY *pkey = EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519, nullptr, raw_private_key.data(), raw_private_key.size());
    if (!pkey) {
        // Consider adding OpenSSL error stack logging here for debugging
        throw std::runtime_error("Failed to create EVP_PKEY from raw private key.");
    }

    // Sign the message
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to create EVP_MD_CTX.");
    }

    // Note: For Ed25519 'pure' signing (no hash), the digest type is NULL.
    if (EVP_DigestSignInit(md_ctx, nullptr, nullptr, nullptr, pkey) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to initialize EVP digest sign context.");
    }

    // Update with the message bytes
    if (EVP_DigestSignUpdate(md_ctx, reinterpret_cast<const unsigned char*>(message_to_sign.data()), message_to_sign.length()) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to update EVP digest sign context.");
    }

    // Determine the signature length
    size_t sig_len = 0;
    if (EVP_DigestSignFinal(md_ctx, nullptr, &sig_len) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to determine signature length.");
    }

    // Allocate buffer and get the signature
    std::vector<unsigned char> signature_bytes(sig_len);
    if (EVP_DigestSignFinal(md_ctx, signature_bytes.data(), &sig_len) <= 0) {
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_free(pkey);
        throw std::runtime_error("Failed to finalize EVP digest sign.");
    }

    // Clean up
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);

    // Encode the signature in Base64
    return base64_encode(signature_bytes.data(), sig_len);
}

std::string RestClient::http_method_to_string(HttpMethod method) {
    switch (method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DELETE: return "DELETE";
        default: throw std::invalid_argument("Invalid HTTP method");
    }
}

size_t RestClient::write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

} // namespace backpack 