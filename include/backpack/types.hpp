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
    USER_BALANCES,
    ORDERS,
    POSITIONS,
    BALANCES
};

// Convert Channel to string
inline std::string channel_to_string(Channel channel) {
    switch (channel) {
        case Channel::TICKER: return "ticker";
        case Channel::TRADES: return "trades";
        case Channel::CANDLES_1M: return "candle.1m";
        case Channel::CANDLES_5M: return "candle.5m";
        case Channel::CANDLES_15M: return "candle.15m";
        case Channel::CANDLES_1H: return "candle.1h";
        case Channel::CANDLES_4H: return "candle.4h";
        case Channel::CANDLES_1D: return "candle.1d";
        case Channel::DEPTH: return "depth";
        case Channel::DEPTH_SNAPSHOT: return "depth";  // Using depth for both since snapshot is initial state
        case Channel::USER_ORDERS: return "orders";
        case Channel::USER_TRADES: return "user.trades";
        case Channel::USER_POSITIONS: return "positions";
        case Channel::USER_BALANCES: return "balances";
        case Channel::ORDERS: return "orders";
        case Channel::POSITIONS: return "positions";
        case Channel::BALANCES: return "balances";
        default: throw std::runtime_error("Unknown channel type");
    }
}

// Convert string to Channel
inline std::optional<Channel> string_to_channel(const std::string& str) {
    if (str == "ticker") return Channel::TICKER;
    if (str == "trades") return Channel::TRADES;
    if (str == "candle.1m") return Channel::CANDLES_1M;
    if (str == "candle.5m") return Channel::CANDLES_5M;
    if (str == "candle.15m") return Channel::CANDLES_15M;
    if (str == "candle.1h") return Channel::CANDLES_1H;
    if (str == "candle.4h") return Channel::CANDLES_4H;
    if (str == "candle.1d") return Channel::CANDLES_1D;
    if (str == "depth") return Channel::DEPTH;
    if (str == "orders") return Channel::USER_ORDERS;
    if (str == "trades") return Channel::USER_TRADES;
    if (str == "positions") return Channel::USER_POSITIONS;
    if (str == "balances") return Channel::USER_BALANCES;
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
        // Convert symbol format from SOL-USDC to SOL_USDC
        std::string formatted_symbol = symbol;
        if (!symbol.empty()) {
            std::replace(formatted_symbol.begin(), formatted_symbol.end(), '-', '_');
        }

        // Create the stream name
        std::string stream = channel_to_string(channel);
        if (!formatted_symbol.empty()) {
            stream += "." + formatted_symbol;
        }

        json j = {
            {"method", "SUBSCRIBE"},
            {"params", json::array({stream})}
        };

        return j;
    }
};

// Unsubscription request
struct UnsubscriptionRequest {
    Channel channel;
    std::string symbol;

    json to_json() const {
        // Convert symbol format from SOL-USDC to SOL_USDC
        std::string formatted_symbol = symbol;
        if (!symbol.empty()) {
            std::replace(formatted_symbol.begin(), formatted_symbol.end(), '-', '_');
        }

        // Create the stream name
        std::string stream = channel_to_string(channel);
        if (!formatted_symbol.empty()) {
            stream += "." + formatted_symbol;
        }

        json j = {
            {"method", "UNSUBSCRIBE"},
            {"params", json::array({stream})}
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
        // Map long and short field names for WebSocket payloads
        if (j.contains("orderId")) order.id = j.at("orderId").get<std::string>();
        else if (j.contains("i")) order.id = j.at("i").get<std::string>();
        else order.id = "";

        order.client_order_id = j.value("clientOrderId", "");

        if (j.contains("symbol")) order.symbol = j.at("symbol").get<std::string>();
        else if (j.contains("s")) order.symbol = j.at("s").get<std::string>();
        else order.symbol = "";

        std::string side_str;
        if (j.contains("side")) side_str = j.at("side").get<std::string>();
        else if (j.contains("S")) side_str = j.at("S").get<std::string>();
        else side_str = "";
        order.side = string_to_order_side(side_str).value_or(OrderSide::BUY);

        std::string type_str;
        if (j.contains("type")) type_str = j.at("type").get<std::string>();
        else if (j.contains("o")) type_str = j.at("o").get<std::string>();
        else type_str = "";
        order.type = string_to_order_type(type_str).value_or(OrderType::LIMIT);

        // Robust parsing for price, quantity, executed_quantity
        auto get_json_field_as_double = [](const json& current_json, const std::string& long_key, const std::string& short_key, double default_val) {
            const json* field_ptr = nullptr;
            if (current_json.contains(long_key)) {
                field_ptr = &current_json.at(long_key);
            } else if (current_json.contains(short_key)) {
                field_ptr = &current_json.at(short_key);
            }

            if (field_ptr) {
                if (field_ptr->is_number()) {
                    return field_ptr->get<double>();
                } else if (field_ptr->is_string()) {
                    try {
                        return std::stod(field_ptr->get<std::string>());
                    } catch (const std::exception& e) {
                        // Optionally log parsing error for string to double
                        // spdlog::warn("Failed to convert string '{}' to double for key '{}': {}", field_ptr->get<std::string>(), short_key, e.what());
                        return default_val;
                    }
                } else if (field_ptr->is_null()) {
                    return default_val;
                }
                // Optionally log unexpected type
                // spdlog::warn("Unexpected type for key '{}': {}", short_key, field_ptr->type_name());
            }
            return default_val;
        };

        order.price = get_json_field_as_double(j, "price", "p", 0.0);
        order.quantity = get_json_field_as_double(j, "quantity", "q", 0.0);
        // Assuming order.executed_quantity refers to base quantity (short key "z" or long key "executedQty")
        // "Q" is executedQuoteQty, which might be different.
        order.executed_quantity = get_json_field_as_double(j, "executedQty", "z", 0.0);

        std::string status_str;
        if (j.contains("status")) status_str = j.at("status").get<std::string>();
        else if (j.contains("X")) status_str = j.at("X").get<std::string>(); // Check for "X" (used in payload)
        else if (j.contains("x")) status_str = j.at("x").get<std::string>(); // Then "x"
        else status_str = "";
        order.status = string_to_order_status(status_str).value_or(OrderStatus::NEW);

        if (j.contains("timestamp")) order.timestamp = j.at("timestamp").get<std::string>(); // Prefer long key if present and string
        else if (j.contains("T")) { // Short key "T" is numeric
            if (j.at("T").is_number()) {
                order.timestamp = std::to_string(j.at("T").get<std::uint64_t>());
            } else if (j.at("T").is_string()) { // Defensive: if T ever comes as string
                 order.timestamp = j.at("T").get<std::string>();
            } else {
                order.timestamp = ""; // Or handle error
            }
        } else {
            order.timestamp = "";
        }
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
