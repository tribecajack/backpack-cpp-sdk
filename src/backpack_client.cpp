#include "backpack/backpack_client.hpp"
#include <iostream>

namespace backpack {

BackpackClient::BackpackClient(const std::string& websocket_url, const std::string& rest_url)
    : ws_client_(std::make_unique<BackpackWebSocketClient>(websocket_url)),
      rest_client_(std::make_unique<RestClient>(rest_url)) {
}

BackpackClient::~BackpackClient() {
    disconnect();
}

void BackpackClient::set_credentials(const std::string& api_key, const std::string& api_secret) {
    ws_client_->set_credentials(api_key, api_secret);
    rest_client_->set_credentials(api_key, api_secret);
}

bool BackpackClient::connect() {
    return ws_client_->connect();
}

void BackpackClient::disconnect() {
    ws_client_->disconnect();
}

bool BackpackClient::is_connected() const {
    return ws_client_->is_connected();
}

bool BackpackClient::authenticate() {
    return ws_client_->authenticate();
}

bool BackpackClient::subscribe_ticker(const std::string& symbol, std::function<void(const Ticker&)> callback) {
    return subscribe_to_channel<Ticker>(Channel::TICKER, symbol, callback);
}

bool BackpackClient::subscribe_trades(const std::string& symbol, std::function<void(const Trade&)> callback) {
    return subscribe_to_channel<Trade>(Channel::TRADES, symbol, callback);
}

bool BackpackClient::subscribe_candles(const std::string& symbol, Channel interval, std::function<void(const Candle&)> callback) {
    // Validate that the interval is a candle channel
    if (interval != Channel::CANDLES_1M && 
        interval != Channel::CANDLES_5M && 
        interval != Channel::CANDLES_15M && 
        interval != Channel::CANDLES_1H && 
        interval != Channel::CANDLES_4H && 
        interval != Channel::CANDLES_1D) {
        std::cerr << "Invalid candle interval channel" << std::endl;
        return false;
    }
    
    return subscribe_to_channel<Candle>(interval, symbol, callback);
}

bool BackpackClient::subscribe_depth(const std::string& symbol, std::function<void(const OrderBook&)> callback) {
    return subscribe_to_channel<OrderBook>(Channel::DEPTH, symbol, callback);
}

bool BackpackClient::subscribe_depth_snapshot(const std::string& symbol, std::function<void(const OrderBook&)> callback) {
    return subscribe_to_channel<OrderBook>(Channel::DEPTH_SNAPSHOT, symbol, callback);
}

bool BackpackClient::subscribe_user_orders(std::function<void(const Order&)> callback) {
    return subscribe_to_channel<Order>(Channel::USER_ORDERS, "", callback);
}

bool BackpackClient::subscribe_user_trades(std::function<void(const Trade&)> callback) {
    return subscribe_to_channel<Trade>(Channel::USER_TRADES, "", callback);
}

bool BackpackClient::subscribe_user_positions(std::function<void(const Position&)> callback) {
    return subscribe_to_channel<Position>(Channel::USER_POSITIONS, "", callback);
}

bool BackpackClient::subscribe_user_balances(std::function<void(const Balance&)> callback) {
    return subscribe_to_channel<Balance>(Channel::USER_BALANCES, "", callback);
}

bool BackpackClient::unsubscribe(Channel channel, const std::string& symbol) {
    return ws_client_->unsubscribe(channel, symbol);
}

void BackpackClient::ping() {
    ws_client_->ping();
}

// REST API methods

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

std::vector<Order> BackpackClient::get_all_orders(const std::string& symbol, int limit, 
                                                 int64_t start_time, int64_t end_time) {
    return rest_client_->get_all_orders(symbol, limit, start_time, end_time);
}

Account BackpackClient::get_account() {
    return rest_client_->get_account();
}

std::vector<Balance> BackpackClient::get_balances() {
    return rest_client_->get_balances();
}

std::vector<Trade> BackpackClient::get_account_trades(const std::string& symbol, int limit,
                                                     int64_t start_time, int64_t end_time) {
    return rest_client_->get_account_trades(symbol, limit, start_time, end_time);
}

} // namespace backpack
