# Backpack C++ SDK

A modern C++ SDK for interacting with the [Backpack Exchange](https://backpack.exchange) WebSocket API. This SDK provides a robust implementation for real-time market data streaming and account management.

## Features

- Real-time market data streaming
  - Ticker updates
  - Order book (depth) updates
  - Trade updates
  - Candlestick data (multiple timeframes)
- Private data streaming (with authentication)
  - Order updates
  - Position updates
  - Balance updates
  - User trade updates
- Robust WebSocket connection handling
  - Automatic reconnection
  - Heartbeat monitoring
  - Error handling
- Modern C++ design
  - Type-safe enums for channels and order types
  - RAII principles
  - Exception safety
  - Thread safety

## Dependencies

- C++17 or later
- CMake 3.12 or later
- OpenSSL
- [nlohmann/json](https://github.com/nlohmann/json)
- [WebSocket++](https://github.com/zaphoyd/websocketpp)
- Boost (for WebSocket++)

## Installation

1. Clone the repository:
```bash
git clone https://github.com/tribecajack/backpack-cpp-sdk.git
cd backpack-cpp-sdk
```

2. Create build directory and build:
```bash
mkdir build
cd build
cmake ..
make
```

## Usage

### WebSocket Market Data Example

```cpp
#include <backpack/backpack_client.hpp>
#include <iostream>

int main() {
    // Create WebSocket client
    backpack::BackpackWebSocketClient client("wss://ws.backpack.exchange");
    
    // Connect to WebSocket server
    if (!client.connect()) {
        std::cerr << "Failed to connect" << std::endl;
        return 1;
    }
    
    // Optional: Set API credentials for authenticated endpoints
    const char* api_key = std::getenv("BACKPACK_API_KEY");
    const char* api_secret = std::getenv("BACKPACK_API_SECRET");
    if (api_key && api_secret) {
        client.set_credentials(api_key, api_secret);
    }
    
    // Subscribe to channels
    const std::string symbol = "SOL-USDC";
    client.subscribe(backpack::Channel::TICKER, symbol);
    client.subscribe(backpack::Channel::TRADES, symbol);
    client.subscribe(backpack::Channel::DEPTH, symbol);
    client.subscribe(backpack::Channel::CANDLES_1M, symbol);
    
    // Register callback for market data
    client.register_general_callback([](const nlohmann::json& msg) {
        std::cout << "Received: " << msg.dump(2) << std::endl;
    });
    
    // Keep connection alive
    while (client.is_connected()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

### Building the Example

```bash
cd build
cmake ..
make
./websocket_example
```

## Available Channels

### Public Channels

- `TICKER`: Real-time price and 24h statistics
- `TRADES`: Real-time trade execution data
- `DEPTH`: Real-time order book updates
- `CANDLES_1M`: 1-minute candlestick data
- `CANDLES_5M`: 5-minute candlestick data
- `CANDLES_15M`: 15-minute candlestick data
- `CANDLES_1H`: 1-hour candlestick data
- `CANDLES_4H`: 4-hour candlestick data
- `CANDLES_1D`: 1-day candlestick data

### Private Channels (Requires Authentication)

- `USER_ORDERS`: Personal order updates
- `USER_TRADES`: Personal trade updates
- `USER_POSITIONS`: Position updates
- `USER_BALANCES`: Balance updates

## Authentication

To use authenticated endpoints, set your API credentials:

1. As environment variables:
```bash
export BACKPACK_API_KEY="your_api_key"
export BACKPACK_API_SECRET="your_api_secret"
```

2. Or programmatically:
```cpp
client.set_credentials("your_api_key", "your_api_secret");
```

## Message Formats

### Ticker Update
```json
{
  "stream": "ticker.SOL_USDC",
  "data": {
    "e": "ticker",
    "s": "SOL_USDC",
    "c": "124.50",
    "h": "130.30",
    "l": "124.19",
    "v": "260656",
    "n": 324595
  }
}
```

### Order Book Update
```json
{
  "stream": "depth.SOL_USDC",
  "data": {
    "e": "depth",
    "s": "SOL_USDC",
    "U": 2060508621,
    "u": 2060508621,
    "b": [["124.20", "6.05"]],
    "a": []
  }
}
```

## Error Handling

The SDK uses exceptions for error handling. Main exception types:

- `BackpackError`: Base exception class
- `ConnectionError`: WebSocket connection issues
- `AuthenticationError`: Invalid credentials
- `SubscriptionError`: Channel subscription failures

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Disclaimer

This is an unofficial SDK and is not affiliated with Backpack Exchange. Use at your own risk.
