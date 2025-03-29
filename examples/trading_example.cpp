#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <mutex>
#include <backpack/backpack_client.hpp>

// Global flag for graceful shutdown
std::atomic<bool> running(true);

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    std::cout << "Received signal " << signal << ", shutting down..." << std::endl;
    running = false;
}

// Format double with fixed precision
std::string format_double(double value, int precision = 8) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

int main(int argc, char* argv[]) {
    // Set up signal handling
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        // Create Backpack client
        backpack::BackpackClient client;
        
        // Check if API keys are provided as command-line arguments
        bool has_credentials = false;
        if (argc >= 3) {
            std::string api_key = argv[1];
            std::string api_secret = argv[2];
            std::cout << "Using provided API credentials" << std::endl;
            client.set_credentials(api_key, api_secret);
            has_credentials = true;
        } else {
            std::cout << "No API credentials provided. Running in public mode only." << std::endl;
            std::cout << "Usage: " << argv[0] << " <api_key> <api_secret>" << std::endl;
        }
        
        // Connect to WebSocket server
        std::cout << "Connecting to Backpack Exchange WebSocket server..." << std::endl;
        if (!client.connect()) {
            std::cerr << "Failed to connect to WebSocket server" << std::endl;
            return 1;
        }
        std::cout << "Connected successfully" << std::endl;
        
        // Authenticate if credentials provided
        if (has_credentials) {
            if (!client.authenticate()) {
                std::cerr << "Authentication failed" << std::endl;
                return 1;
            }
            std::cout << "Authentication successful" << std::endl;
        }
        
        // Get server time (REST API)
        int64_t server_time = client.get_server_time();
        std::cout << "Server time: " << server_time << " (" 
                  << backpack::timestamp_to_iso8601(server_time) << ")" << std::endl;
        
        // Get exchange information (REST API)
        backpack::ExchangeInfo exchange_info = client.get_exchange_info();
        std::cout << "Exchange information:" << std::endl;
        std::cout << "  Timezone: " << exchange_info.timezone << std::endl;
        std::cout << "  Server time: " << exchange_info.server_time << std::endl;
        std::cout << "  Symbols: " << exchange_info.symbols.size() << std::endl;
        
        // Print information for SOL-USDC symbol
        for (const auto& symbol_info : exchange_info.symbols) {
            if (symbol_info.name == "SOL-USDC") {
                std::cout << "SOL-USDC Info:" << std::endl;
                std::cout << "  Base asset: " << symbol_info.base_asset << std::endl;
                std::cout << "  Quote asset: " << symbol_info.quote_asset << std::endl;
                std::cout << "  Active: " << (symbol_info.is_active ? "Yes" : "No") << std::endl;
                std::cout << "  Min price: " << symbol_info.min_price << std::endl;
                std::cout << "  Max price: " << symbol_info.max_price << std::endl;
                std::cout << "  Tick size: " << symbol_info.tick_size << std::endl;
                std::cout << "  Min quantity: " << symbol_info.min_qty << std::endl;
                std::cout << "  Max quantity: " << symbol_info.max_qty << std::endl;
                std::cout << "  Step size: " << symbol_info.step_size << std::endl;
                break;
            }
        }
        
        // Get SOL-USDC ticker (REST API)
        backpack::Ticker ticker = client.get_ticker("SOL-USDC");
        std::cout << "SOL-USDC Ticker:" << std::endl;
        std::cout << "  Last price: " << ticker.last_price << std::endl;
        std::cout << "  Best bid: " << ticker.best_bid << std::endl;
        std::cout << "  Best ask: " << ticker.best_ask << std::endl;
        std::cout << "  24h volume: " << ticker.volume_24h << std::endl;
        std::cout << "  24h price change: " << ticker.price_change_24h << "%" << std::endl;
        
        // Get SOL-USDC order book (REST API)
        backpack::OrderBook order_book = client.get_order_book("SOL-USDC", 5);
        std::cout << "SOL-USDC Order Book (top 5 levels):" << std::endl;
        
        std::cout << "  Bids:" << std::endl;
        for (size_t i = 0; i < order_book.bids.size() && i < 5; ++i) {
            std::cout << "    " << format_double(order_book.bids[i].price) 
                      << " : " << format_double(order_book.bids[i].quantity) << std::endl;
        }
        
        std::cout << "  Asks:" << std::endl;
        for (size_t i = 0; i < order_book.asks.size() && i < 5; ++i) {
            std::cout << "    " << format_double(order_book.asks[i].price) 
                      << " : " << format_double(order_book.asks[i].quantity) << std::endl;
        }
        
        // Get recent trades (REST API)
        std::vector<backpack::Trade> trades = client.get_recent_trades("SOL-USDC", 5);
        std::cout << "SOL-USDC Recent Trades (last 5):" << std::endl;
        for (const auto& trade : trades) {
            std::cout << "  " << trade.id << ": Price: " << format_double(trade.price) 
                      << ", Quantity: " << format_double(trade.quantity) 
                      << ", Buyer maker: " << (trade.is_buyer_maker ? "Yes" : "No") << std::endl;
        }
        
        // If authenticated, get account information
        if (has_credentials) {
            // Get account information (REST API)
            backpack::Account account = client.get_account();
            std::cout << "Account Information:" << std::endl;
            std::cout << "  Account ID: " << account.account_id << std::endl;
            std::cout << "  Account Type: " << account.account_type << std::endl;
            std::cout << "  Can Trade: " << (account.can_trade ? "Yes" : "No") << std::endl;
            std::cout << "  Can Withdraw: " << (account.can_withdraw ? "Yes" : "No") << std::endl;
            
            // Get balances (REST API)
            std::vector<backpack::Balance> balances = client.get_balances();
            std::cout << "Account Balances:" << std::endl;
            for (const auto& balance : balances) {
                if (balance.free > 0 || balance.locked > 0) {
                    std::cout << "  " << balance.asset << ": Free: " << format_double(balance.free) 
                              << ", Locked: " << format_double(balance.locked) << std::endl;
                }
            }
            
            // Get open orders (REST API)
            std::vector<backpack::Order> open_orders = client.get_open_orders();
            std::cout << "Open Orders:" << std::endl;
            if (open_orders.empty()) {
                std::cout << "  No open orders" << std::endl;
            } else {
                for (const auto& order : open_orders) {
                    std::cout << "  Order " << order.id << " (" << order.symbol << "): " 
                              << backpack::order_side_to_string(order.side) << " "
                              << backpack::order_type_to_string(order.type) << " at "
                              << format_double(order.price) << ", Quantity: " 
                              << format_double(order.quantity) << ", Executed: " 
                              << format_double(order.executed_quantity) << std::endl;
                }
            }
            
            // Subscribe to user orders updates (WebSocket)
            std::mutex order_mutex;
            client.subscribe_user_orders([&order_mutex](const backpack::Order& order) {
                std::lock_guard<std::mutex> lock(order_mutex);
                std::cout << "Order Update: " << order.id << " (" << order.symbol << ") "
                          << backpack::order_status_to_string(order.status) << std::endl;
            });
            
            // Subscribe to user balances updates (WebSocket)
            std::mutex balance_mutex;
            client.subscribe_user_balances([&balance_mutex](const backpack::Balance& balance) {
                std::lock_guard<std::mutex> lock(balance_mutex);
                std::cout << "Balance Update: " << balance.asset << " Free: " 
                          << format_double(balance.free) << ", Locked: " 
                          << format_double(balance.locked) << std::endl;
            });
        }
        
        // Subscribe to ticker updates for SOL-USDC (WebSocket)
        std::mutex ticker_mutex;
        client.subscribe_ticker("SOL-USDC", [&ticker_mutex](const backpack::Ticker& ticker) {
            std::lock_guard<std::mutex> lock(ticker_mutex);
            std::cout << "Ticker Update [" << ticker.symbol << "] "
                      << "Last price: " << format_double(ticker.last_price) << std::endl;
        });
        
        // Subscribe to trade updates for SOL-USDC (WebSocket)
        std::mutex trade_mutex;
        client.subscribe_trades("SOL-USDC", [&trade_mutex](const backpack::Trade& trade) {
            std::lock_guard<std::mutex> lock(trade_mutex);
            std::cout << "Trade: " << trade.id << " Price: " << format_double(trade.price) 
                      << ", Quantity: " << format_double(trade.quantity) << std::endl;
        });
        
        // Set up ping to keep connection alive
        std::thread ping_thread([&client]() {
            while (running) {
                if (client.is_connected()) {
                    client.ping();
                }
                std::this_thread::sleep_for(std::chrono::seconds(30));
            }
        });
        
        // If authenticated, provide a simple market making example
        if (has_credentials) {
            std::cout << "\nSimple Market Making Example:" << std::endl;
            std::cout << "This example will place orders around the current price." << std::endl;
            std::cout << "Press Ctrl+C to exit." << std::endl;
            
            // Get current SOL-USDC market price
            ticker = client.get_ticker("SOL-USDC");
            double mid_price = (ticker.best_bid + ticker.best_ask) / 2.0;
            
            // Place a buy order slightly below market price
            backpack::OrderRequest buy_order;
            buy_order.symbol = "SOL-USDC";
            buy_order.side = backpack::OrderSide::BUY;
            buy_order.type = backpack::OrderType::LIMIT;
            buy_order.price = mid_price * 0.99;  // 1% below mid price
            buy_order.quantity = 0.1;  // Small quantity for example
            
            // Place a sell order slightly above market price
            backpack::OrderRequest sell_order;
            sell_order.symbol = "SOL-USDC";
            sell_order.side = backpack::OrderSide::SELL;
            sell_order.type = backpack::OrderType::LIMIT;
            sell_order.price = mid_price * 1.01;  // 1% above mid price
            sell_order.quantity = 0.1;  // Small quantity for example
            
            try {
                // Test the orders first (without actually placing them)
                if (client.test_order(buy_order) && client.test_order(sell_order)) {
                    std::cout << "Order tests passed. You can uncomment the next lines to actually place orders." << std::endl;
                    
                    /*
                    // Uncomment to actually place orders
                    backpack::Order placed_buy_order = client.create_order(buy_order);
                    std::cout << "Placed buy order: " << placed_buy_order.id << " at " 
                              << format_double(placed_buy_order.price) << std::endl;
                    
                    backpack::Order placed_sell_order = client.create_order(sell_order);
                    std::cout << "Placed sell order: " << placed_sell_order.id << " at " 
                              << format_double(placed_sell_order.price) << std::endl;
                    */
                } else {
                    std::cout << "Order tests failed. Check your account balance and order parameters." << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error placing orders: " << e.what() << std::endl;
            }
        }
        
        // Main loop - keep program running until signal is received
        std::cout << "\nWebSocket client running. Press Ctrl+C to exit." << std::endl;
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