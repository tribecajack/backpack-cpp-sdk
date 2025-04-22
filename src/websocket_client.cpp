#include "backpack/websocket_client.hpp"
#include <iostream>
#include <sstream>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <cstring>

namespace backpack {

BackpackWebSocketClient::BackpackWebSocketClient(const std::string& base_url)
    : base_url_(base_url),
      connected_(false),
      authenticated_(false),
      running_(false) {
    
    // Clear access and error logs to reduce output noise
    client_.clear_access_channels(websocketpp::log::alevel::all);
    client_.clear_error_channels(websocketpp::log::elevel::all);
    
    // Set up error logging
    client_.set_access_channels(websocketpp::log::alevel::connect | 
                              websocketpp::log::alevel::disconnect | 
                              websocketpp::log::alevel::app);
    client_.set_error_channels(websocketpp::log::elevel::all);
    
    // Initialize ASIO
    client_.init_asio();
    
    // Register handlers
    client_.set_tls_init_handler([this](const auto& hdl) {
        return this->on_tls_init();
    });
    
    client_.set_open_handler([this](const auto& hdl) {
        this->on_open(hdl);
    });
    
    client_.set_close_handler([this](const auto& hdl) {
        this->on_close(hdl);
    });
    
    client_.set_message_handler([this](const auto& hdl, const auto& msg) {
        this->on_message(hdl, msg);
    });
    
    client_.set_fail_handler([this](const auto& hdl) {
        this->on_error(hdl, websocketpp::lib::error_code());
    });
}

BackpackWebSocketClient::~BackpackWebSocketClient() {
    disconnect();
}

void BackpackWebSocketClient::set_credentials(const std::string& api_key, const std::string& api_secret) {
    std::lock_guard<std::mutex> lock(mutex_);
    credentials_.api_key = api_key;
    credentials_.api_secret = api_secret;
}

bool BackpackWebSocketClient::connect() {
    if (is_connected()) {
        return true;
    }
    
    try {
        websocketpp::lib::error_code ec;
        WebsocketClient::connection_ptr con = client_.get_connection(base_url_, ec);
        
        if (ec) {
            std::cerr << "Could not create connection: " << ec.message() << std::endl;
            return false;
        }

        // Add required headers for signed requests
        if (credentials_.is_valid()) {
            auto timestamp = std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count());
            
            // Default window to 5000ms
            const std::string window = "5000";
            
            // Create the signature base string
            std::string signature_base = timestamp + "GET/ws" + window;
            
            // Add headers
            con->append_header("X-Timestamp", timestamp);
            con->append_header("X-Window", window);
            con->append_header("X-API-Key", credentials_.api_key);
            con->append_header("X-Signature", generate_signature(signature_base, credentials_.api_secret));
        }
        
        connection_handle_ = con->get_handle();
        client_.connect(con);
        
        // Start the client thread if not already running
        if (!running_) {
            running_ = true;
            client_thread_ = std::thread(&BackpackWebSocketClient::run_client, this);
            send_thread_ = std::thread(&BackpackWebSocketClient::process_message_queue, this);
        }
        
        // Wait for connection to be established
        for (int i = 0; i < 50; ++i) {  // Wait up to 5 seconds
            if (connected_) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        return connected_;
        
    } catch (const std::exception& e) {
        std::cerr << "Connection error: " << e.what() << std::endl;
        return false;
    }
}

void BackpackWebSocketClient::disconnect() {
    if (!is_connected()) {
        return;
    }
    
    try {
        websocketpp::lib::error_code ec;
        client_.close(connection_handle_, websocketpp::close::status::normal, "Disconnecting", ec);
        
        if (ec) {
            std::cerr << "Error during disconnect: " << ec.message() << std::endl;
        }
        
        connected_ = false;
        authenticated_ = false;
        
        // Stop client thread if running
        if (running_) {
            running_ = false;
            
            // Notify the message queue processor to exit
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                queue_condition_.notify_all();
            }
            
            // Join threads if they're joinable
            if (client_thread_.joinable()) {
                client_thread_.join();
            }
            
            if (send_thread_.joinable()) {
                send_thread_.join();
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error during disconnect: " << e.what() << std::endl;
    }
}

bool BackpackWebSocketClient::is_connected() const {
    return connected_;
}

bool BackpackWebSocketClient::authenticate() {
    if (!is_connected()) {
        return false;
    }
    
    if (!credentials_.is_valid()) {
        std::cerr << "Cannot authenticate: Invalid credentials" << std::endl;
        return false;
    }
    
    try {
        json auth_payload = generate_auth_payload(credentials_.api_key, credentials_.api_secret);
        std::string auth_message = auth_payload.dump();
        
        if (!send_message(auth_message)) {
            return false;
        }
        
        // Wait for authentication to be confirmed
        for (int i = 0; i < 50; ++i) {  // Wait up to 5 seconds
            if (authenticated_) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        return authenticated_;
        
    } catch (const std::exception& e) {
        std::cerr << "Authentication error: " << e.what() << std::endl;
        return false;
    }
}

bool BackpackWebSocketClient::subscribe(Channel channel, const std::string& symbol) {
    if (!is_connected()) {
        return false;
    }
    
    // Check if authentication is required for this channel
    bool auth_required = false;
    if (channel == Channel::USER_BALANCES || 
        channel == Channel::USER_ORDERS || 
        channel == Channel::USER_POSITIONS || 
        channel == Channel::USER_TRADES) {
        
        auth_required = true;
        
        if (!authenticated_ && !authenticate()) {
            std::cerr << "Cannot subscribe to " << channel_to_string(channel) 
                      << ": Authentication failed" << std::endl;
            return false;
        }
    }
    
    try {
        SubscriptionRequest req;
        req.channel = channel;
        req.symbol = symbol;
        req.auth_required = auth_required;
        
        json sub_payload = req.to_json();
        std::string sub_message = sub_payload.dump();
        
        return send_message(sub_message);
        
    } catch (const std::exception& e) {
        std::cerr << "Subscription error: " << e.what() << std::endl;
        return false;
    }
}

bool BackpackWebSocketClient::unsubscribe(Channel channel, const std::string& symbol) {
    if (!is_connected()) {
        return false;
    }
    
    try {
        UnsubscriptionRequest req;
        req.channel = channel;
        req.symbol = symbol;
        
        json unsub_payload = req.to_json();
        std::string unsub_message = unsub_payload.dump();
        
        return send_message(unsub_message);
        
    } catch (const std::exception& e) {
        std::cerr << "Unsubscription error: " << e.what() << std::endl;
        return false;
    }
}

void BackpackWebSocketClient::register_callback(Channel channel, const std::string& symbol, MessageCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::string key = get_callback_key(channel, symbol);
    callbacks_[key] = callback;
}

void BackpackWebSocketClient::register_general_callback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    general_callback_ = callback;
}

void BackpackWebSocketClient::ping() {
    if (!is_connected()) {
        return;
    }
    
    try {
        json ping_payload = {
            {"method", "PING"}
        };
        std::string ping_message = ping_payload.dump();
        
        send_message(ping_message);
        
    } catch (const std::exception& e) {
        std::cerr << "Ping error: " << e.what() << std::endl;
    }
}

websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> BackpackWebSocketClient::on_tls_init() {
    auto ctx = websocketpp::lib::make_shared<websocketpp::lib::asio::ssl::context>(
        websocketpp::lib::asio::ssl::context::sslv23_client
    );
    
    try {
        ctx->set_options(
            websocketpp::lib::asio::ssl::context::default_workarounds |
            websocketpp::lib::asio::ssl::context::no_sslv2 |
            websocketpp::lib::asio::ssl::context::no_sslv3 |
            websocketpp::lib::asio::ssl::context::single_dh_use
        );
        
        ctx->set_verify_mode(websocketpp::lib::asio::ssl::verify_none);
        
    } catch (const std::exception& e) {
        std::cerr << "Error in TLS init: " << e.what() << std::endl;
    }
    
    return ctx;
}

void BackpackWebSocketClient::on_open(WebsocketConnectionHdl hdl) {
    connected_ = true;
}

void BackpackWebSocketClient::on_close(WebsocketConnectionHdl hdl) {
    connected_ = false;
    authenticated_ = false;
}

void BackpackWebSocketClient::on_message(WebsocketConnectionHdl hdl, WebsocketClient::message_ptr msg) {
    try {
        // Parse the message
        json message = json::parse(msg->get_payload());
        
        // Check if it's an auth response
        if (message.contains("type") && message["type"] == "auth") {
            if (message.contains("success") && message["success"].get<bool>()) {
                authenticated_ = true;
            } else {
                std::string error = message.value("message", "Unknown authentication error");
                std::cerr << "Authentication failed: " << error << std::endl;
            }
            return;
        }
        
        // Call the general callback if registered
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (general_callback_) {
                general_callback_(message);
            }
        }
        
        // Handle the message based on its content
        handle_message(message);
        
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }
}

void BackpackWebSocketClient::on_error(WebsocketConnectionHdl hdl, const websocketpp::lib::error_code& ec) {
    std::cerr << "WebSocket error: " << (ec ? ec.message() : "Unknown error");
    if (client_.get_con_from_hdl(hdl)) {
        std::cerr << " (Error code: " << client_.get_con_from_hdl(hdl)->get_ec() << ")";
    }
    std::cerr << std::endl;
    
    connected_ = false;
    authenticated_ = false;
}

void BackpackWebSocketClient::run_client() {
    try {
        client_.run();
    } catch (const std::exception& e) {
        std::cerr << "Client thread error: " << e.what() << std::endl;
    }
    
    running_ = false;
}

void BackpackWebSocketClient::process_message_queue() {
    while (running_) {
        std::string message;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_condition_.wait(lock, [this] {
                return !running_ || !message_queue_.empty();
            });
            
            if (!running_ && message_queue_.empty()) {
                break;
            }
            
            if (!message_queue_.empty()) {
                message = message_queue_.front();
                message_queue_.pop();
            }
        }
        
        if (!message.empty()) {
            try {
                websocketpp::lib::error_code ec;
                client_.send(connection_handle_, message, websocketpp::frame::opcode::text, ec);
                
                if (ec) {
                    std::cerr << "Error sending message: " << ec.message() << std::endl;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "Error sending message: " << e.what() << std::endl;
            }
        }
    }
}

bool BackpackWebSocketClient::send_message(const std::string& message) {
    if (!is_connected()) {
        return false;
    }
    
    try {
        // Log outgoing message
        std::cout << "Sending message: " << message << std::endl;
        
        std::lock_guard<std::mutex> lock(queue_mutex_);
        message_queue_.push(message);
        queue_condition_.notify_one();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error queueing message: " << e.what() << std::endl;
        return false;
    }
}

std::string BackpackWebSocketClient::get_callback_key(Channel channel, const std::string& symbol) {
    std::ostringstream oss;
    oss << channel_to_string(channel);
    
    if (!symbol.empty()) {
        oss << ":" << symbol;
    }
    
    return oss.str();
}

void BackpackWebSocketClient::handle_message(const json& message) {
    try {
        // Log raw message for debugging
        std::cout << "Received message: " << message.dump(2) << std::endl;
        
        // Check if it's an error message
        if (message.contains("error")) {
            std::cerr << "Error from server: " << message["error"].dump() << std::endl;
            return;
        }
        
        // Extract stream and data from the message
        if (!message.contains("stream") || !message.contains("data")) {
            return;
        }
        
        std::string stream = message["stream"];
        const json& data = message["data"];
        
        // Parse the stream name to get channel and symbol
        std::string channel_str;
        std::string symbol;
        
        size_t dot_pos = stream.find('.');
        if (dot_pos != std::string::npos) {
            channel_str = stream.substr(0, dot_pos);
            symbol = stream.substr(dot_pos + 1);
            // Convert symbol format from SOL_USDC back to SOL-USDC
            std::replace(symbol.begin(), symbol.end(), '_', '-');
        } else {
            channel_str = stream;
        }
        
        auto channel_opt = string_to_channel(channel_str);
        if (!channel_opt) {
            std::cerr << "Unknown channel in stream: " << stream << std::endl;
            return;
        }
        
        Channel channel = *channel_opt;
        
        // Call specific callback if registered
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = get_callback_key(channel, symbol);
        
        auto it = callbacks_.find(key);
        if (it != callbacks_.end() && it->second) {
            it->second(data);  // Pass only the data part
        } else {
            // Try with just the channel (for all symbols)
            key = get_callback_key(channel, "");
            it = callbacks_.find(key);
            if (it != callbacks_.end() && it->second) {
                it->second(data);  // Pass only the data part
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

namespace backpack {


// All functionality lives in backpack::WebSocketClient’s header.
// This translation unit exists only to give the library a .cpp file
// so the build system always has at least one source file.  
// Add out‑of‑line definitions here later if you move code out of the header.

main

// Helper function to generate HMAC-SHA256 signature
std::string BackpackWebSocketClient::generate_signature(const std::string& message, const std::string& secret) {
    unsigned char* digest = HMAC(EVP_sha256(),
                                secret.c_str(), secret.length(),
                                reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
                                nullptr, nullptr);
    
    char hex[65];
    for (int i = 0; i < 32; i++) {
        sprintf(hex + i * 2, "%02x", digest[i]);
    }
    hex[64] = 0;
    
    return std::string(hex);
}

} // namespace backpack
