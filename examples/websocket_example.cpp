#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <backpack/websocket_client.hpp>
#include <backpack/types.hpp>

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
        backpack::WebSocketClient client;
        
        // Connect to WebSocket server
        std::cout << "Connecting to Backpack Exchange WebSocket server..." << std::endl;
        if (!client.connect("wss://ws.backpack.exchange")) {
            std::cerr << "Failed to connect to WebSocket server" << std::endl;
            return 1;
        }
        std::cout << "Connected successfully" << std::endl;
        
        // Subscribe to public channels for SOL-USDC
        const std::string symbol = "SOL-USDC";
        bool subscribed = true;
        
        // Track subscription status
        struct ChannelStatus {
            bool ticker = false;
            bool trades = false;
            bool depth = false;
        } status;
        
        // Register callback to handle market data
        client.set_message_handler([&status](const std::string& msg) {
            std::cout << "Received: " << msg << std::endl;
            
            // You can add more specific message handling here if needed
            if (msg.find("ticker") != std::string::npos) {
                status.ticker = true;
            } else if (msg.find("trades") != std::string::npos) {
                status.trades = true;
            } else if (msg.find("depth") != std::string::npos) {
                status.depth = true;
            }
            
            // Print status
            std::cout << "\nChannel Status:\n"
                      << "  Ticker: " << (status.ticker ? "✓" : "✗") << "\n"
                      << "  Trades: " << (status.trades ? "✓" : "✗") << "\n"
                      << "  Depth: " << (status.depth ? "✓" : "✗") << "\n"
                      << std::endl;
        });

        // Subscribe to channels
        std::cout << "Subscribing to public channels..." << std::endl;
        
        // Subscribe to ticker
        backpack::SubscriptionRequest ticker_req{backpack::Channel::TICKER, symbol};
        client.send(ticker_req.to_json().dump());
        
        // Subscribe to trades
        backpack::SubscriptionRequest trades_req{backpack::Channel::TRADES, symbol};
        client.send(trades_req.to_json().dump());
        
        // Subscribe to order book updates
        backpack::SubscriptionRequest depth_req{backpack::Channel::DEPTH, symbol};
        client.send(depth_req.to_json().dump());
        
        // Main loop - keep program running until signal is received
        std::cout << "WebSocket client running. Press Ctrl+C to exit." << std::endl;
        
        while (running) {
            if (!client.is_connected()) {
                std::cerr << "Connection lost!" << std::endl;
                break;
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
