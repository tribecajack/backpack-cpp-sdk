#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <backpack/backpack_client.hpp>

// Global flag for graceful shutdown
std::atomic<bool> running(true);

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        // Create Backpack client
        backpack::BackpackClient client;
        
        // Connect to WebSocket server
        std::cout << "Connecting to Backpack Exchange WebSocket server..." << std::endl;
        if (!client.connect()) {
            std::cerr << "Failed to connect to WebSocket server" << std::endl;
            return 1;
        }
        std::cout << "Connected successfully" << std::endl;
        
        // Set API credentials (optional, only needed for authenticated channels)
        // client.set_credentials("your-api-key", "your-api-secret");
        
        // Subscribe to ticker data for SOL-USDC
        client.subscribe_ticker("SOL-USDC", [](const backpack::Ticker& ticker) {
            std::cout << "Ticker [" << ticker.symbol << "] "
                      << "Last price: " << ticker.last_price << ", "
                      << "Best bid: " << ticker.best_bid << ", "
                      << "Best ask: " << ticker.best_ask << ", "
                      << "24h change: " << ticker.price_change_24h << "%, "
                      << "24h volume: " << ticker.volume_24h
                      << std::endl;
        });
        
        // Subscribe to trades for SOL-USDC
        client.subscribe_trades("SOL-USDC", [](const backpack::Trade& trade) {
            std::cout << "Trade [" << trade.symbol << "] "
                      << "ID: " << trade.id << ", "
                      << "Price: " << trade.price << ", "
                      << "Quantity: " << trade.quantity << ", "
                      << "Buyer maker: " << (trade.is_buyer_maker ? "Yes" : "No") << ", "
                      << "Timestamp: " << trade.timestamp
                      << std::endl;
        });
        
        // Subscribe to 1m candles for SOL-USDC
        client.subscribe_candles("SOL-USDC", backpack::Channel::CANDLES_1M, [](const backpack::Candle& candle) {
            std::cout << "Candle [" << candle.symbol << "] "
                      << "Time: " << candle.timestamp << ", "
                      << "Open: " << candle.open << ", "
                      << "High: " << candle.high << ", "
                      << "Low: " << candle.low << ", "
                      << "Close: " << candle.close << ", "
                      << "Volume: " << candle.volume
                      << std::endl;
        });
        
        // Subscribe to depth snapshot for SOL-USDC (full order book)
        client.subscribe_depth_snapshot("SOL-USDC", [](const backpack::OrderBook& book) {
            std::cout << "Order Book [" << book.symbol << "]" << std::endl;
            
            std::cout << "Bids:" << std::endl;
            for (size_t i = 0; i < std::min<size_t>(5, book.bids.size()); ++i) {
                std::cout << "  " << std::fixed << std::setprecision(8) 
                          << book.bids[i].price << " : " 
                          << book.bids[i].quantity << std::endl;
            }
            
            std::cout << "Asks:" << std::endl;
            for (size_t i = 0; i < std::min<size_t>(5, book.asks.size()); ++i) {
                std::cout << "  " << std::fixed << std::setprecision(8) 
                          << book.asks[i].price << " : " 
                          << book.asks[i].quantity << std::endl;
            }
            
            std::cout << std::endl;
        });
        
        // Subscribe to authenticated channels (if credentials provided)
        /*
        // Subscribe to user orders
        client.subscribe_user_orders([](const backpack::Order& order) {
            std::cout << "Order [" << order.symbol << "] "
                      << "ID: " << order.id << ", "
                      << "Type: " << backpack::order_type_to_string(order.type) << ", "
                      << "Side: " << backpack::order_side_to_string(order.side) << ", "
                      << "Price: " << order.price << ", "
                      << "Quantity: " << order.quantity << ", "
                      << "Status: " << backpack::order_status_to_string(order.status)
                      << std::endl;
        });
        
        // Subscribe to user balances
        client.subscribe_user_balances([](const backpack::Balance& balance) {
            std::cout << "Balance [" << balance.asset << "] "
                      << "Free: " << balance.free << ", "
                      << "Locked: " << balance.locked
                      << std::endl;
        });
        */
        
        // Set up ping to keep connection alive
        std::thread ping_thread([&client]() {
            while (running) {
                if (client.is_connected()) {
                    client.ping();
                }
                std::this_thread::sleep_for(std::chrono::seconds(30));
            }
        });
        
        // Main loop - keep program running until signal is received
        std::cout << "WebSocket client running. Press Ctrl+C to exit." << std::endl;
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        // Clean up
        std::cout << "Disconnecting..." << std::endl;
        client.disconnect();
        
        if (ping_thread.joinable()) {
            ping_thread.join();
        }
        
        std::cout << "Shutdown complete" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
