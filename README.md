# Backpack Exchange C++ SDK

A C++ SDK for interacting with the Backpack Exchange API. This SDK provides easy access to market data streams, user data streams, and trading functionality through both WebSocket and REST API connections.

## Features

- WebSocket client for real-time data:
  - Market data (tickers, trades, order book, candles)
  - User data (orders, trades, positions, balances)
- REST API client for trading:
  - Market data retrieval
  - Order management (create, cancel, query)
  - Account information
  - Trading history
- Flexible callback system for handling real-time updates
- Thread-safe implementation
- Comprehensive type system for Backpack Exchange data structures

## Dependencies

- C++17 or newer
- OpenSSL
- CURL
- Boost (for websocketpp)
- WebSocket++ (automatically downloaded via CMake)
- nlohmann_json (automatically downloaded via CMake if not found)

## Building

### Prerequisites

Make sure you have the following installed:
- CMake (version 3.14 or newer)
- A C++17 compatible compiler (GCC, Clang, MSVC)
- OpenSSL development libraries
- CURL development libraries
- Boost development libraries

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/tribecajack/backpack-cpp-sdk.git
cd backpack-cpp-sdk

# Create a build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j$(nproc)

# Install (optional)
sudo make install
```

## Usage

### WebSocket API Example

Here's a simple example of using the SDK to subscribe to various data streams:

```cpp
#include <iostream>
#include <backpack/backpack_client.hpp>
#include <thread>
#include <chrono>

int main() {
    // Create client instance
    backpack::BackpackClient client;
    
    // Connect to WebSocket server
    if (!client.connect()) {
        std::cerr << "Failed to connect to WebSocket server" << std::endl;
        return 1;
    }
    
    // Optional: Set API credentials for authenticated endpoints
    // client.set_credentials("your-api-key", "your-api-secret");
    
    // Subscribe to ticker data
    client.subscribe_ticker("SOL-USDC", [](const backpack::Ticker& ticker) {
        std::cout << "Ticker: " << ticker.symbol << " Last price: " << ticker.last_price << std::endl;
    });
    
    // Subscribe to trade data
    client.subscribe_trades("SOL-USDC", [](const backpack::Trade& trade) {
        std::cout << "Trade: " << trade.symbol << " Price: " << trade.price 
                  << " Quantity: " << trade.quantity << std::endl;
    });
    
    // Keep the program running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

### REST API Example

Here's an example of using the REST API for trading:

```cpp
#include <iostream>
#include <backpack/backpack_client.hpp>

int main() {
    // Create client instance
    backpack::BackpackClient client;
    
    // Set API credentials (required for trading)
    client.set_credentials("your-api-key", "your-api-secret");
    
    try {
        // Get market data
        backpack::Ticker ticker = client.get_ticker("SOL-USDC");
        std::cout << "SOL-USDC Last Price: " << ticker.last_price << std::endl;
        
        // Place a limit buy order
        backpack::OrderRequest buy_order;
        buy_order.symbol = "SOL-USDC";
        buy_order.side = backpack::OrderSide::BUY;
        buy_order.type = backpack::OrderType::LIMIT;
        buy_order.price = ticker.last_price * 0.95;  // 5% below current price
        buy_order.quantity = 1.0;
        
        // Test the order first
        if (client.test_order(buy_order)) {
            // Place the actual order
            backpack::Order placed_order = client.create_order(buy_order);
            std::cout << "Order placed: ID=" << placed_order.id 
                      << ", Status=" << backpack::order_status_to_string(placed_order.status) 
                      << std::endl;
            
            // Get all open orders
            std::vector<backpack::Order> open_orders = client.get_open_orders();
            std::cout << "Open Orders: " << open_orders.size() << std::endl;
            
            // Cancel the order
            if (client.cancel_order("SOL-USDC", placed_order.id)) {
                std::cout << "Order canceled successfully" << std::endl;
            }
        } else {
            std::cout << "Order validation failed" << std::endl;
        }
        
        // Get account balances
        std::vector<backpack::Balance> balances = client.get_balances();
        for (const auto& balance : balances) {
            if (balance.free > 0 || balance.locked > 0) {
                std::cout << "Balance: " << balance.asset 
                          << " Free: " << balance.free 
                          << " Locked: " << balance.locked 
                          << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

### Authenticated WebSocket Endpoints

For authenticated WebSocket endpoints, you need to set your API credentials and authenticate:

```cpp
// Set API credentials
client.set_credentials("your-api-key", "your-api-secret");

// Authenticate connection
if (!client.authenticate()) {
    std::cerr << "Authentication failed" << std::endl;
    return 1;
}

// Subscribe to user orders
client.subscribe_user_orders([](const backpack::Order& order) {
    std::cout << "Order: " << order.id << " Status: " 
              << backpack::order_status_to_string(order.status) << std::endl;
});

// Subscribe to user balances
client.subscribe_user_balances([](const backpack::Balance& balance) {
    std::cout << "Balance: " << balance.asset << " Free: " << balance.free 
              << " Locked: " << balance.locked << std::endl;
});
```

## Examples

See the `examples` directory for complete working examples:

- `websocket_example.cpp`: Demonstrates WebSocket subscriptions for market data

## API Documentation

### Main Classes

- `BackpackClient`: High-level client for interacting with Backpack Exchange (both WebSocket and REST)
- `BackpackWebSocketClient`: Low-level WebSocket client handling the WebSocket connections
- `RestClient`: Low-level REST client handling the HTTP requests
- Data structures: `Ticker`, `Trade`, `Candle`, `OrderBook`, `Order`, `Balance`, `Position`, etc.

### WebSocket Channels

Available WebSocket channels:

- `Channel::TICKER`: Real-time ticker data
- `Channel::TRADES`: Real-time trade data
- `Channel::CANDLES_1M`: 1-minute candle data
- `Channel::CANDLES_5M`: 5-minute candle data
- `Channel::CANDLES_15M`: 15-minute candle data
- `Channel::CANDLES_1H`: 1-hour candle data
- `Channel::CANDLES_4H`: 4-hour candle data
- `Channel::CANDLES_1D`: 1-day candle data
- `Channel::DEPTH`: Order book updates
- `Channel::DEPTH_SNAPSHOT`: Full order book snapshots
- `Channel::USER_ORDERS`: User order updates (authenticated)
- `Channel::USER_TRADES`: User trade updates (authenticated)
- `Channel::USER_POSITIONS`: User position updates (authenticated)
- `Channel::USER_BALANCES`: User balance updates (authenticated)

### REST API Endpoints

The SDK supports the following REST API endpoints:

#### Public Endpoints

- `get_server_time()`: Get server time
- `get_exchange_info()`: Get exchange information
- `get_ticker(symbol)`: Get ticker for a symbol
- `get_all_tickers()`: Get tickers for all symbols
- `get_order_book(symbol, limit)`: Get order book for a symbol
- `get_recent_trades(symbol, limit)`: Get recent trades for a symbol
- `get_historical_trades(symbol, limit, from_id)`: Get historical trades for a symbol
- `get_candles(symbol, interval, limit, start_time, end_time)`: Get candles for a symbol

#### Private Endpoints (require authentication)

- `create_order(order)`: Create a new order
- `test_order(order)`: Test an order without placing it
- `cancel_order(symbol, order_id)`: Cancel an order
- `cancel_order_by_client_id(symbol, client_order_id)`: Cancel an order by client order ID
- `cancel_all_orders(symbol)`: Cancel all open orders for a symbol
- `get_order(symbol, order_id)`: Get an order by ID
- `get_order_by_client_id(symbol, client_order_id)`: Get an order by client order ID
- `get_open_orders(symbol)`: Get all open orders
- `get_all_orders(symbol, limit, start_time, end_time)`: Get all orders (open and closed)
- `get_account()`: Get account information
- `get_balances()`: Get account balances
- `get_account_trades(symbol, limit, start_time, end_time)`: Get account trades

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## References

- [Backpack Exchange Documentation](https://docs.backpack.exchange/)
- [Backpack Exchange WebSocket API](https://docs.backpack.exchange/websockets)
- [Backpack Exchange REST API](https://docs.backpack.exchange/rest-api)
