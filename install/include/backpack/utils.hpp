#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <nlohmann/json.hpp>

namespace backpack {

using json = nlohmann::json;

/**
 * @brief Get current timestamp in milliseconds since epoch
 * @return Current timestamp in milliseconds
 */
inline int64_t get_current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

/**
 * @brief Convert timestamp milliseconds to ISO8601 string
 * @param timestamp_ms Timestamp in milliseconds
 * @return ISO8601 formatted timestamp string
 */
inline std::string timestamp_to_iso8601(int64_t timestamp_ms) {
    char buffer[30];
    time_t seconds = timestamp_ms / 1000;
    int milliseconds = timestamp_ms % 1000;
    
    std::tm* tm_info = std::gmtime(&seconds);
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", tm_info);
    
    std::ostringstream oss;
    oss << buffer << '.' << std::setfill('0') << std::setw(3) << milliseconds << 'Z';
    return oss.str();
}

/**
 * @brief Generate HMAC-SHA256 signature for a message
 * @param message Message to sign
 * @param secret Secret key for signing
 * @return Hex-encoded signature
 */
inline std::string generate_hmac_sha256(const std::string& message, const std::string& secret) {
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hash_len;
    
    HMAC(EVP_sha256(), secret.c_str(), static_cast<int>(secret.length()),
         reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
         hash, &hash_len);
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < hash_len; ++i) {
        oss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }
    
    return oss.str();
}

/**
 * @brief Generate signature for Backpack API request
 * @param api_secret API secret key
 * @param timestamp Timestamp in milliseconds
 * @param window Signature validity window in milliseconds (default 5000)
 * @return Signature for authentication
 */
inline std::string generate_signature(const std::string& api_secret, int64_t timestamp, int64_t window = 5000) {
    std::string timestamp_str = std::to_string(timestamp);
    std::string window_str = std::to_string(window);
    std::string message = timestamp_str + window_str;
    
    return generate_hmac_sha256(message, api_secret);
}

/**
 * @brief Generate authentication payload for websocket connection
 * @param api_key API key
 * @param api_secret API secret
 * @return Authentication payload as JSON object
 */
inline json generate_auth_payload(const std::string& api_key, const std::string& api_secret) {
    int64_t timestamp = get_current_timestamp_ms();
    int64_t window = 5000; // 5 seconds validity
    
    std::string signature = generate_signature(api_secret, timestamp, window);
    
    json auth_payload = {
        {"type", "auth"},
        {"key", api_key},
        {"timestamp", timestamp},
        {"window", window},
        {"signature", signature}
    };
    
    return auth_payload;
}

/**
 * @brief URL encode a string
 * @param str Input string to encode
 * @return URL encoded string
 */
inline std::string url_encode(const std::string& str) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    
    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
        }
    }
    
    return escaped.str();
}

/**
 * @brief Build query string from parameters
 * @param params Parameters map
 * @return URL encoded query string
 */
inline std::string build_query_string(const std::map<std::string, std::string>& params) {
    std::ostringstream oss;
    bool first = true;
    
    for (const auto& param : params) {
        if (!first) {
            oss << "&";
        }
        oss << url_encode(param.first) << "=" << url_encode(param.second);
        first = false;
    }
    
    return oss.str();
}

/**
 * @brief Base64 encode data
 * @param input Input data to encode
 * @return Base64 encoded string
 */
inline std::string base64_encode(const std::string& input) {
    const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    std::string result;
    int i = 0;
    unsigned char char_array_3[3], char_array_4[4];
    
    const unsigned char* bytes_to_encode = reinterpret_cast<const unsigned char*>(input.c_str());
    size_t in_len = input.length();
    
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for (i = 0; i < 4; i++) {
                result += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }
    
    if (i) {
        for (int j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        
        for (int j = 0; j < i + 1; j++) {
            result += base64_chars[char_array_4[j]];
        }
        
        while (i++ < 3) {
            result += '=';
        }
    }
    
    return result;
}

} // namespace backpack
