#include "backpack/backpack_client.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

namespace backpack {

using json = nlohmann::json;

BackpackClient::BackpackClient(const std::string& websocket_url, const std::string& rest_url)
    : websocket_url_(websocket_url)
    , rest_url_(rest_url)
    , ws_client_(std::make_unique<WebSocketClient>())
    , rest_client_(std::make_unique<RestClient>(rest_url)) {
}

BackpackClient::~BackpackClient() {
    disconnect();
}

void BackpackClient::set_credentials(const std::string& api_key, const std::string& api_secret) {
    api_key_ = api_key;
    api_secret_ = api_secret;
    rest_client_->set_credentials(api_key, api_secret);
}

bool BackpackClient::connect() {
    if (connected_) {
        return true;
    }

    // Set up WebSocket handlers
    ws_client_->set_open_handler([this]() {
        std::lock_guard<std::mutex> lock(mutex_);
        connected_ = true;
    });

    ws_client_->set_close_handler([this]() {
        std::lock_guard<std::mutex> lock(mutex_);
        connected_ = false;
        authenticated_ = false;
    });

    ws_client_->set_message_handler([this](const std::string& message) {
        try {
            json j = json::parse(message);
            if (j.contains("type")) {
                std::string type = j["type"];
                if (type == "error") {
                    std::cerr << "WebSocket error: " << j["message"].get<std::string>() << std::endl;
                    return;
                }
                
                if (type == "authenticated") {
                    std::lock_guard<std::mutex> lock(mutex_);
                    authenticated_ = true;
                    return;
                }
                
                if (type == "subscribed" || type == "unsubscribed") {
                    return;
                }
                
                if (j.contains("data")) {
                    std::string channel = j["channel"].get<std::string>();
                    std::string symbol = j.value("symbol", "");
                    std::string key = channel + ":" + symbol;
                    
                    auto it = message_handlers_.find(key);
                    if (it != message_handlers_.end()) {
                        it->second(j["data"]);
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing message: " << e.what() << std::endl;
        }
    });

    // Connect to WebSocket server
    return ws_client_->connect(websocket_url_);
}

void BackpackClient::disconnect() {
    if (!connected_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = false;
    authenticated_ = false;
    message_handlers_.clear();
}

bool BackpackClient::is_connected() const {
    return connected_;
}

bool BackpackClient::authenticate() {
    if (!connected_ || authenticated_ || api_key_.empty() || api_secret_.empty()) {
        return false;
    }
    
    try {
        json auth = {
            {"type", "auth"},
            {"key", api_key_},
            {"secret", api_secret_}
        };
        
        ws_client_->send(auth.dump());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Authentication error: " << e.what() << std::endl;
        return false;
    }
}

template<typename T>
bool BackpackClient::subscribe_to_channel(Channel channel, const std::string& symbol, std::function<void(const T&)> callback) {
    if (!connected_) {
        return false;
    }
    
    try {
        std::string channel_str = channel_to_string(channel);
        std::string key = channel_str + ":" + symbol;
        
        // Store message handler
        message_handlers_[key] = [callback](const json& data) {
            try {
                T obj = T::from_json(data);
                callback(obj);
            } catch (const std::exception& e) {
                std::cerr << "Error parsing message: " << e.what() << std::endl;
            }
        };
        
        // Send subscription request
        json sub = {
            {"type", "subscribe"},
            {"channel", channel_str}
        };
        
        if (!symbol.empty()) {
            sub["symbol"] = symbol;
        }
        
        ws_client_->send(sub.dump());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Subscription error: " << e.what() << std::endl;
        return false;
    }
}

bool BackpackClient::unsubscribe(Channel channel, const std::string& symbol) {
    if (!connected_) {
        return false;
    }
    
    try {
        std::string channel_str = channel_to_string(channel);
        std::string key = channel_str + ":" + symbol;
        
        // Remove message handler
        message_handlers_.erase(key);
        
        // Send unsubscription request
        json unsub = {
            {"type", "unsubscribe"},
            {"channel", channel_str}
        };
        
        if (!symbol.empty()) {
            unsub["symbol"] = symbol;
        }
        
        ws_client_->send(unsub.dump());
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Unsubscription error: " << e.what() << std::endl;
        return false;
    }
}

void BackpackClient::ping() {
    if (connected_) {
        ws_client_->send(R"({"type":"ping"})");
    }
}

// REST API method implementations remain unchanged
int64_t BackpackClient::get_server_time() {
    return rest_client_->get_server_time();
}

ExchangeInfo BackpackClient::get_exchange_info() {
    return rest_client_->get_exchange_info();
}

Ticker BackpackClient::get_ticker(const std::string& symbol) {
    return rest_client_->get_ticker(symbol);
}

std::map<std::string, Ticker> BackpackClient::get_all_tickers() {
    return rest_client_->get_all_tickers();
}

OrderBook BackpackClient::get_order_book(const std::string& symbol, int limit) {
    return rest_client_->get_order_book(symbol, limit);
}

std::vector<Trade> BackpackClient::get_recent_trades(const std::string& symbol, int limit) {
    return rest_client_->get_recent_trades(symbol, limit);
}

std::vector<Trade> BackpackClient::get_historical_trades(const std::string& symbol, int limit, const std::string& from_id) {
    return rest_client_->get_historical_trades(symbol, limit, from_id);
}

std::vector<Candle> BackpackClient::get_candles(const std::string& symbol, Channel interval, 
                                               int limit, int64_t start_time, int64_t end_time) {
    return rest_client_->get_candles(symbol, interval, limit, start_time, end_time);
}

Order BackpackClient::create_order(const OrderRequest& order) {
    return rest_client_->create_order(order);
}

bool BackpackClient::test_order(const OrderRequest& order) {
    return rest_client_->test_order(order);
}

bool BackpackClient::cancel_order(const std::string& symbol, const std::string& order_id) {
    return rest_client_->cancel_order(symbol, order_id);
}

bool BackpackClient::cancel_order_by_client_id(const std::string& symbol, const std::string& client_order_id) {
    return rest_client_->cancel_order_by_client_id(symbol, client_order_id);
}

int BackpackClient::cancel_all_orders(const std::string& symbol) {
    return rest_client_->cancel_all_orders(symbol);
}

Order BackpackClient::get_order(const std::string& symbol, const std::string& order_id) {
    return rest_client_->get_order(symbol, order_id);
}

Order BackpackClient::get_order_by_client_id(const std::string& symbol, const std::string& client_order_id) {
    return rest_client_->get_order_by_client_id(symbol, client_order_id);
}

std::vector<Order> BackpackClient::get_open_orders(const std::string& symbol) {
    return rest_client_->get_open_orders(symbol);
}

std::vector<Order> BackpackClient::get_all_orders(const std::string& symbol, int limit, const std::string& from_id) {
    return rest_client_->get_all_orders(symbol, limit, from_id);
}

Account BackpackClient::get_account() {
    return rest_client_->get_account();
}

std::vector<Balance> BackpackClient::get_balances() {
    return rest_client_->get_balances();
}

std::vector<Trade> BackpackClient::get_account_trades(const std::string& symbol, int limit, const std::string& from_id) {
    return rest_client_->get_account_trades(symbol, limit, from_id);
}

} // namespace backpack
