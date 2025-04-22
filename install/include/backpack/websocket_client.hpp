#pragma once

#include <string>
#include <queue>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "ix/IXWebSocket.h"
#include "websocket_error.hpp"

namespace backpack {

class WebSocketClient {
public:
    using MessageHandler = std::function<void(const std::string&)>;
    using ErrorHandler = std::function<void(const std::string&)>;
    using OpenHandler = std::function<void()>;
    using CloseHandler = std::function<void()>;

    static constexpr size_t MAX_QUEUE_SIZE = 1000;
    static constexpr int MAX_RETRY_ATTEMPTS = 5;
    static constexpr uint32_t CONNECTION_TIMEOUT_MS = 5000;

    WebSocketClient();
    ~WebSocketClient();

    bool connect(const std::string& uri, const std::string& api_key, const std::string& api_secret);
    bool send(const std::string& message);
    void close();
    bool is_connected() const;

    void set_message_handler(MessageHandler handler) { m_message_handler = std::move(handler); }
    void set_fail_handler(ErrorHandler handler) { m_fail_handler = std::move(handler); }
    void set_open_handler(OpenHandler handler) { m_open_handler = std::move(handler); }
    void set_close_handler(CloseHandler handler) { m_close_handler = std::move(handler); }

private:
    void process_message_queue();
    bool send_with_retry(const std::string& message, int retries);
    bool reconnect();

    ix::WebSocket m_ws;
    std::queue<std::string> m_message_queue;
    std::mutex m_queue_mutex;
    
    MessageHandler m_message_handler;
    ErrorHandler m_fail_handler;
    OpenHandler m_open_handler;
    CloseHandler m_close_handler;

    std::string m_uri;
    std::string m_api_key;
    std::string m_api_secret;

    bool m_running;
};

} // namespace backpack
