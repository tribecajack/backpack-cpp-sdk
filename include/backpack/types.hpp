#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace backpack {

using json = nlohmann::json;

// Subscription channels
enum class Channel {
    TICKER,
    TRADES,
    CANDLES_1M,
    CANDLES_5M, 
    CANDLES_15M,
    CANDLES_1H,
    CANDLES_4H,
    CANDLES_1D,
    DEPTH,
    DEPTH_SNAPSHOT,
    USER_ORDERS,
    USER_TRADES,
    USER_POSITIONS,
    USER_BALANCES
};

// Convert Channel to string
inline std::string channel_to_string(Channel channel) {
    switch (channel) {
        case Channel::TICKER: return "ticker";
        case Channel::TRADES: return "trades";
        case Channel::CANDLES_1M: return "candles1m";
        case Channel::CANDLES_5M: return "candles5m";
        case Channel::CANDLES_15M: return "candles15m";
        case Channel::CANDLES_1H: return "candles1h";
        case Channel::CANDLES_4H: return "candles4h";
        case Channel::CANDLES_1D: return "candles1d";
        case Channel::DEPTH: return "depth";
        case Channel::DEPTH_SNAPSHOT: return "depthSnapshot";
        case Channel::USER_ORDERS: return "userOrders";
        case Channel::USER_TRADES: return "userTrades";
        case Channel::USER_POSITIONS: return "userPositions";
        case Channel::USER_BALANCES: return "userBalances";
        default: return "unknown";
    }
}

// Convert string to Channel
inline std::optional<Channel> string_to_channel(const std::string& str) {
    if (str == "ticker") return Channel::TICKER;
    if (str == "trades") return Channel::TRADES;
    if (str == "candles1m") return Channel::CANDLES_1M;
    if (str == "candles5m") return Channel::CANDLES_5M;
    if (str == "candles15m") return Channel::CANDLES_15M;
    if (str == "candles1h") return Channel::CANDLES_1H;
    if (str == "candles4h") return Channel::CANDLES_4H;
    if (str == "candles1d") return Channel::CANDLES_1D;
    if (str == "depth") return Channel::DEPTH;
    if (str == "depthSnapshot") return Channel::DEPTH_SNAPSHOT;
    if (str == "userOrders") return Channel::USER_ORDERS;
    if (str == "userTrades") return Channel::USER_TRADES;
    if (str == "userPositions") return Channel::USER_POSITIONS;
    if (str == "userBalances") return Channel::USER_BALANCES;
    return std::nullopt;
}

// Event types
enum class EventType {
    SUBSCRIBE,
    UNSUBSCRIBE,
    PING,
    PONG,
    ERROR,
    DATA
};

// Convert EventType to string
inline std::string event_type_to_string(EventType event_type) {
    switch (event_type) {
        case EventType::SUBSCRIBE: return "subscribe";
        case EventType::UNSUBSCRIBE: return "unsubscribe";
        case EventType::PING: return "ping";
        case EventType::PONG: return "pong";
        case EventType::ERROR: return "error";
        case EventType::DATA: return "data";
        default: return "unknown";
    }
}

// Convert string to EventType
inline std::optional<EventType> string_to_event_type(const std::string& str) {
    if (str == "subscribe") return EventType::SUBSCRIBE;
    if (str == "unsubscribe") return EventType::UNSUBSCRIBE;
    if (str == "ping") return EventType::PING;
    if (str == "pong") return EventType::PONG;
    if (str == "error") return EventType::ERROR;
    if (str == "data") return EventType::DATA;
    return std::nullopt;
}

// Subscription request
struct SubscriptionRequest {
    Channel channel;
    std::string symbol;
    bool auth_required = false;

    json to_json() const {
        json j = {
            {"type", "subscribe"},
            {"channel", channel_to_string(channel)},
            {"symbol", symbol}
        };
        return j;
    }
};

// Unsubscription request
struct UnsubscriptionRequest {
    Channel channel;
    std::string symbol;

    json to_json() const {
        json j = {
            {"type", "unsubscribe"},
            {"channel", channel_to_string(channel)},
            {"symbol", symbol}
        };
        return j;
    }
};

// API credentials
struct Credentials {
    std::string api_key; // Public key
    std::string base64_private_key; // Base64 encoded private key

    bool is_valid() const {
        return !api_key.empty() && !base64_private_key.empty();
    }
};

// Websocket message callback type
using MessageCallback = std::function<void(json)>;

// Ticker data structure
struct Ticker {
    std::string symbol;
    std::string timestamp;
    double last_price;
    double best_bid;
    double best_ask;
    double volume_24h;
    double price_change_24h;
    
    static Ticker from_json(const json& j) {
        Ticker ticker;
        ticker.symbol = j.at("symbol");
        ticker.timestamp = j.at("timestamp");
        ticker.last_price = std::stod(j.at("lastPrice").get<std::string>());
        ticker.best_bid = std::stod(j.at("bestBid").get<std::string>());
        ticker.best_ask = std::stod(j.at("bestAsk").get<std::string>());
        ticker.volume_24h = std::stod(j.at("volume24h").get<std::string>());
        ticker.price_change_24h = std::stod(j.at("priceChange24h").get<std::string>());
        return ticker;
    }
};

// Order book level
struct OrderBookLevel {
    double price;
    double quantity;
    
    static OrderBookLevel from_json(const json& j) {
        OrderBookLevel level;
        level.price = std::stod(j[0].get<std::string>());
        level.quantity = std::stod(j[1].get<std::string>());
        return level;
    }
};

// Order book
struct OrderBook {
    std::string symbol;
    std::vector<OrderBookLevel> bids;
    std::vector<OrderBookLevel> asks;
    
    static OrderBook from_json(const json& j) {
        OrderBook book;
        book.symbol = j.at("symbol");
        
        for (const auto& bid : j.at("bids")) {
            book.bids.push_back(OrderBookLevel::from_json(bid));
        }
        
        for (const auto& ask : j.at("asks")) {
            book.asks.push_back(OrderBookLevel::from_json(ask));
        }
        
        return book;
    }
};

// Trade
struct Trade {
    std::string symbol;
    std::string id;
    std::string timestamp;
    double price;
    double quantity;
    bool is_buyer_maker;
    
    static Trade from_json(const json& j) {
        Trade trade;
        trade.symbol = j.at("symbol");
        trade.id = j.at("id");
        trade.timestamp = j.at("timestamp");
        trade.price = std::stod(j.at("price").get<std::string>());
        trade.quantity = std::stod(j.at("quantity").get<std::string>());
        trade.is_buyer_maker = j.at("isBuyerMaker").get<bool>();
        return trade;
    }
};

// Candle
struct Candle {
    std::string symbol;
    std::string timestamp;
    double open;
    double high;
    double low;
    double close;
    double volume;
    
    static Candle from_json(const json& j) {
        Candle candle;
        candle.symbol = j.at("symbol");
        candle.timestamp = j.at("timestamp");
        candle.open = std::stod(j.at("open").get<std::string>());
        candle.high = std::stod(j.at("high").get<std::string>());
        candle.low = std::stod(j.at("low").get<std::string>());
        candle.close = std::stod(j.at("close").get<std::string>());
        candle.volume = std::stod(j.at("volume").get<std::string>());
        return candle;
    }
};

// Order types
enum class OrderType {
    LIMIT,
    MARKET,
    STOP_LOSS,
    TAKE_PROFIT
};

// Convert OrderType to string
inline std::string order_type_to_string(OrderType order_type) {
    switch (order_type) {
        case OrderType::LIMIT: return "LIMIT";
        case OrderType::MARKET: return "MARKET";
        case OrderType::STOP_LOSS: return "STOP_LOSS";
        case OrderType::TAKE_PROFIT: return "TAKE_PROFIT";
        default: return "UNKNOWN";
    }
}

// Convert string to OrderType
inline std::optional<OrderType> string_to_order_type(const std::string& str) {
    if (str == "LIMIT") return OrderType::LIMIT;
    if (str == "MARKET") return OrderType::MARKET;
    if (str == "STOP_LOSS") return OrderType::STOP_LOSS;
    if (str == "TAKE_PROFIT") return OrderType::TAKE_PROFIT;
    return std::nullopt;
}

// Order sides
enum class OrderSide {
    BUY,
    SELL
};

// Convert OrderSide to string
inline std::string order_side_to_string(OrderSide order_side) {
    switch (order_side) {
        case OrderSide::BUY: return "BUY";
        case OrderSide::SELL: return "SELL";
        default: return "UNKNOWN";
    }
}

// Convert string to OrderSide
inline std::optional<OrderSide> string_to_order_side(const std::string& str) {
    if (str == "BUY") return OrderSide::BUY;
    if (str == "SELL") return OrderSide::SELL;
    return std::nullopt;
}

// Order status
enum class OrderStatus {
    NEW,
    PARTIALLY_FILLED,
    FILLED,
    CANCELED,
    REJECTED
};

// Convert OrderStatus to string
inline std::string order_status_to_string(OrderStatus order_status) {
    switch (order_status) {
        case OrderStatus::NEW: return "NEW";
        case OrderStatus::PARTIALLY_FILLED: return "PARTIALLY_FILLED";
        case OrderStatus::FILLED: return "FILLED";
        case OrderStatus::CANCELED: return "CANCELED";
        case OrderStatus::REJECTED: return "REJECTED";
        default: return "UNKNOWN";
    }
}

// Convert string to OrderStatus
inline std::optional<OrderStatus> string_to_order_status(const std::string& str) {
    if (str == "NEW") return OrderStatus::NEW;
    if (str == "PARTIALLY_FILLED") return OrderStatus::PARTIALLY_FILLED;
    if (str == "FILLED") return OrderStatus::FILLED;
    if (str == "CANCELED") return OrderStatus::CANCELED;
    if (str == "REJECTED") return OrderStatus::REJECTED;
    return std::nullopt;
}

// Order
struct Order {
    std::string id;
    std::string client_order_id;
    std::string symbol;
    OrderSide side;
    OrderType type;
    double price;
    double quantity;
    double executed_quantity;
    OrderStatus status;
    std::string timestamp;
    
    static Order from_json(const json& j) {
        Order order;
        order.id = j.at("orderId");
        order.client_order_id = j.value("clientOrderId", "");
        order.symbol = j.at("symbol");
        
        auto side_str = j.at("side").get<std::string>();
        auto side_opt = string_to_order_side(side_str);
        order.side = side_opt.value_or(OrderSide::BUY);
        
        auto type_str = j.at("type").get<std::string>();
        auto type_opt = string_to_order_type(type_str);
        order.type = type_opt.value_or(OrderType::LIMIT);
        
        order.price = std::stod(j.at("price").get<std::string>());
        order.quantity = std::stod(j.at("quantity").get<std::string>());
        order.executed_quantity = std::stod(j.at("executedQty").get<std::string>());
        
        auto status_str = j.at("status").get<std::string>();
        auto status_opt = string_to_order_status(status_str);
        order.status = status_opt.value_or(OrderStatus::NEW);
        
        order.timestamp = j.at("timestamp");
        return order;
    }
};

// Balance
struct Balance {
    std::string asset;
    double free;
    double locked;
    
    static Balance from_json(const json& j) {
        Balance balance;
        balance.asset = j.at("asset");
        balance.free = std::stod(j.at("free").get<std::string>());
        balance.locked = std::stod(j.at("locked").get<std::string>());
        return balance;
    }
};

// Position
struct Position {
    std::string symbol;
    double size;
    double entry_price;
    double mark_price;
    double unrealized_pnl;
    
    static Position from_json(const json& j) {
        Position position;
        position.symbol = j.at("symbol");
        position.size = std::stod(j.at("size").get<std::string>());
        position.entry_price = std::stod(j.at("entryPrice").get<std::string>());
        position.mark_price = std::stod(j.at("markPrice").get<std::string>());
        position.unrealized_pnl = std::stod(j.at("unrealizedPnl").get<std::string>());
        return position;
    }
};

// Time in force (TIF)
enum class TimeInForce {
    GTC,  // Good Till Canceled
    IOC,  // Immediate or Cancel
    FOK   // Fill or Kill
};

// Convert TimeInForce to string
inline std::string time_in_force_to_string(TimeInForce tif) {
    switch (tif) {
        case TimeInForce::GTC: return "GTC";
        case TimeInForce::IOC: return "IOC";
        case TimeInForce::FOK: return "FOK";
        default: return "UNKNOWN";
    }
}

// Convert string to TimeInForce
inline std::optional<TimeInForce> string_to_time_in_force(const std::string& str) {
    if (str == "GTC") return TimeInForce::GTC;
    if (str == "IOC") return TimeInForce::IOC;
    if (str == "FOK") return TimeInForce::FOK;
    return std::nullopt;
}

// Order request
struct OrderRequest {
    std::string symbol;
    OrderSide side;
    OrderType type;
    double quantity;
    double price = 0.0;
    std::string client_order_id = "";
    TimeInForce time_in_force = TimeInForce::GTC;
    
    json to_json() const {
        json j = {
            {"symbol", symbol},
            {"side", order_side_to_string(side)},
            {"type", order_type_to_string(type)},
            {"quantity", std::to_string(quantity)},
            {"timeInForce", time_in_force_to_string(time_in_force)}
        };
        
        if (price > 0.0) {
            j["price"] = std::to_string(price);
        }
        
        if (!client_order_id.empty()) {
            j["clientOrderId"] = client_order_id;
        }
        
        return j;
    }
};

// Symbol information
struct SymbolInfo {
    std::string name;
    std::string base_asset;
    std::string quote_asset;
    bool is_active;
    double min_price;
    double max_price;
    double tick_size;
    double min_qty;
    double max_qty;
    double step_size;
    
    static SymbolInfo from_json(const json& j) {
        SymbolInfo info;
        info.name = j.at("symbol");
        info.base_asset = j.at("baseAsset");
        info.quote_asset = j.at("quoteAsset");
        info.is_active = j.at("isActive").get<bool>();
        info.min_price = std::stod(j.at("minPrice").get<std::string>());
        info.max_price = std::stod(j.at("maxPrice").get<std::string>());
        info.tick_size = std::stod(j.at("tickSize").get<std::string>());
        info.min_qty = std::stod(j.at("minQty").get<std::string>());
        info.max_qty = std::stod(j.at("maxQty").get<std::string>());
        info.step_size = std::stod(j.at("stepSize").get<std::string>());
        return info;
    }
};

// Exchange information
struct ExchangeInfo {
    std::string timezone;
    int64_t server_time;
    std::vector<SymbolInfo> symbols;
    
    static ExchangeInfo from_json(const json& j) {
        ExchangeInfo info;
        info.timezone = j.at("timezone");
        info.server_time = j.at("serverTime").get<int64_t>();
        
        for (const auto& symbol_json : j.at("symbols")) {
            info.symbols.push_back(SymbolInfo::from_json(symbol_json));
        }
        
        return info;
    }
};

// Account information
struct Account {
    std::string account_id;
    std::string account_type;
    bool can_trade;
    bool can_withdraw;
    std::vector<Balance> balances;
    
    static Account from_json(const json& j) {
        Account account;
        account.account_id = j.at("accountId");
        account.account_type = j.at("accountType");
        account.can_trade = j.at("canTrade").get<bool>();
        account.can_withdraw = j.at("canWithdraw").get<bool>();
        
        for (const auto& balance_json : j.at("balances")) {
            account.balances.push_back(Balance::from_json(balance_json));
        }
        
        return account;
    }
};

} // namespace backpack
