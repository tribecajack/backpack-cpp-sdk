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
        // Create WebSocket client
        backpack::BackpackWebSocketClient client("wss://ws.backpack.exchange");
        
        // Connect to WebSocket server
        std::cout << "Connecting to Backpack Exchange WebSocket server..." << std::endl;
        if (!client.connect()) {
            std::cerr << "Failed to connect to WebSocket server" << std::endl;
            return 1;
        }
        std::cout << "Connected successfully" << std::endl;
        
        // Get API credentials from environment variables
        const char* api_key = std::getenv("BACKPACK_API_KEY");
        const char* api_secret = std::getenv("BACKPACK_API_SECRET");
        bool authenticated = false;
        
        if (api_key && api_secret) {
            std::cout << "Setting API credentials..." << std::endl;
            client.set_credentials(api_key, api_secret);
            authenticated = true;
        } else {
            std::cout << "No API credentials found in environment variables" << std::endl;
            std::cout << "Set BACKPACK_API_KEY and BACKPACK_API_SECRET to use authenticated endpoints" << std::endl;
            std::cout << "Continuing with public channels only..." << std::endl;
        }
        
        // Subscribe to multiple channels for SOL-USDC
        const std::string symbol = "SOL-USDC";
        bool subscribed = true;
        
        // Subscribe to public channels
        std::cout << "Subscribing to public channels..." << std::endl;
        
        // Subscribe to ticker
        subscribed &= client.subscribe(backpack::Channel::TICKER, symbol);
        
        // Subscribe to trades
        subscribed &= client.subscribe(backpack::Channel::TRADES, symbol);
        
        // Subscribe to different timeframe candles
        subscribed &= client.subscribe(backpack::Channel::CANDLES_1M, symbol);
        subscribed &= client.subscribe(backpack::Channel::CANDLES_5M, symbol);
        subscribed &= client.subscribe(backpack::Channel::CANDLES_15M, symbol);
        
        // Subscribe to order book updates
        subscribed &= client.subscribe(backpack::Channel::DEPTH, symbol);
        
        // Subscribe to authenticated channels if credentials are available
        if (authenticated) {
            std::cout << "Subscribing to authenticated channels..." << std::endl;
            
            // Subscribe to user orders
            subscribed &= client.subscribe(backpack::Channel::USER_ORDERS, "");
            
            // Subscribe to user positions
            subscribed &= client.subscribe(backpack::Channel::USER_POSITIONS, "");
            
            // Subscribe to user balances
            subscribed &= client.subscribe(backpack::Channel::USER_BALANCES, "");
            
            // Subscribe to user trades
            subscribed &= client.subscribe(backpack::Channel::USER_TRADES, "");
        }
        
        if (!subscribed) {
            std::cerr << "Failed to subscribe to one or more channels" << std::endl;
            return 1;
        }
        
        // Track subscription status
        struct ChannelStatus {
            // Public channels
            bool ticker = false;
            bool trades = false;
            bool candles = false;
            bool order_book = false;
            
            // Private channels
            bool user_orders = false;
            bool user_positions = false;
            bool user_balances = false;
            bool user_trades = false;
        } status;
        
        // Register callback to handle market data
        client.register_general_callback([&status, authenticated](const nlohmann::json& msg) {
            if (!msg.contains("stream")) {
                return;
            }
            
            std::string stream = msg["stream"];
            
            // Update status based on received data
            if (stream.find("ticker") != std::string::npos) {
                status.ticker = true;
            } else if (stream.find("trades") != std::string::npos && stream.find("user") == std::string::npos) {
                status.trades = true;
            } else if (stream.find("candle") != std::string::npos) {
                status.candles = true;
            } else if (stream.find("depth") != std::string::npos) {
                status.order_book = true;
            } else if (stream.find("orders") != std::string::npos) {
                status.user_orders = true;
                // Print order updates
                if (msg.contains("data")) {
                    const auto& data = msg["data"];
                    std::cout << "Order Update: " << data.dump(2) << std::endl;
                }
            } else if (stream.find("positions") != std::string::npos) {
                status.user_positions = true;
                // Print position updates
                if (msg.contains("data")) {
                    const auto& data = msg["data"];
                    std::cout << "Position Update: " << data.dump(2) << std::endl;
                }
            } else if (stream.find("balances") != std::string::npos) {
                status.user_balances = true;
                // Print balance updates
                if (msg.contains("data")) {
                    const auto& data = msg["data"];
                    std::cout << "Balance Update: " << data.dump(2) << std::endl;
                }
            } else if (stream.find("user.trades") != std::string::npos) {
                status.user_trades = true;
                // Print user trade updates
                if (msg.contains("data")) {
                    const auto& data = msg["data"];
                    std::cout << "User Trade: " << data.dump(2) << std::endl;
                }
            }
            
            // Print status periodically
            std::cout << "Waiting for data... Status:\n"
                      << "Public Channels:\n"
                      << "  Ticker: " << (status.ticker ? "✓" : "✗") << "\n"
                      << "  Trades: " << (status.trades ? "✓" : "✗") << "\n"
                      << "  Candles: " << (status.candles ? "✓" : "✗") << "\n"
                      << "  Order Book: " << (status.order_book ? "✓" : "✗");
            
            if (authenticated) {
                std::cout << "\nPrivate Channels:\n"
                          << "  Orders: " << (status.user_orders ? "✓" : "✗") << "\n"
                          << "  Positions: " << (status.user_positions ? "✓" : "✗") << "\n"
                          << "  Balances: " << (status.user_balances ? "✓" : "✗") << "\n"
                          << "  User Trades: " << (status.user_trades ? "✓" : "✗");
            }
            std::cout << std::endl;
        });
        
        // Set up ping to keep connection alive
        std::thread ping_thread([&client]() {
            while (running) {
                if (client.is_connected()) {
                    client.ping();
                } else {
                    std::cerr << "Lost connection!" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::seconds(30));
            }
        });
        
        // Main loop - keep program running until signal is received
        std::cout << "WebSocket client running. Press Ctrl+C to exit." << std::endl;
        
        // Monitor subscription status
        int check_count = 0;
        while (running && check_count < 30) {  // Wait up to 30 seconds for initial data
            if (!client.is_connected()) {
                std::cerr << "Connection lost!" << std::endl;
                break;
            }
            
            bool all_public_ready = status.ticker && status.trades && 
                                  status.candles && status.order_book;
                                  
            bool all_private_ready = !authenticated || 
                                   (status.user_orders && status.user_positions && 
                                    status.user_balances && status.user_trades);
                                    
            if (!all_public_ready || !all_private_ready) {
                std::cout << "Waiting for data... Status:\n"
                          << "Public Channels:\n"
                          << "  Ticker: " << (status.ticker ? "✓" : "✗") << "\n"
                          << "  Trades: " << (status.trades ? "✓" : "✗") << "\n"
                          << "  Candles: " << (status.candles ? "✓" : "✗") << "\n"
                          << "  Order Book: " << (status.order_book ? "✓" : "✗");
                
                if (authenticated) {
                    std::cout << "\nPrivate Channels:\n"
                              << "  Orders: " << (status.user_orders ? "✓" : "✗") << "\n"
                              << "  Positions: " << (status.user_positions ? "✓" : "✗") << "\n"
                              << "  Balances: " << (status.user_balances ? "✓" : "✗") << "\n"
                              << "  User Trades: " << (status.user_trades ? "✓" : "✗");
                }
                std::cout << std::endl;
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
            check_count++;
        }
        
        while (running) {
            if (!client.is_connected()) {
                std::cerr << "Connection lost!" << std::endl;
                break;
            }
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
