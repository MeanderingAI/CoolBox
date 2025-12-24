#include "services/url_shortener/url_shortener.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>

namespace services {

URLShortener::URLShortener() : rng_(std::random_device{}()) {}

std::string URLShortener::shorten_url(const std::string& long_url, const std::string& custom_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!is_valid_url(long_url)) {
        return "";
    }
    
    // Check if URL already exists
    for (const auto& [code, url_info] : url_map_) {
        if (url_info.original_url == long_url) {
            return code;
        }
    }
    
    std::string short_code;
    
    if (!custom_code.empty()) {
        // Use custom code if provided and available
        if (url_map_.find(custom_code) != url_map_.end()) {
            return ""; // Custom code already taken
        }
        short_code = custom_code;
    } else {
        // Generate random code
        do {
            short_code = generate_short_code();
        } while (url_map_.find(short_code) != url_map_.end());
    }
    
    ShortenedURL url_info;
    url_info.short_code = short_code;
    url_info.original_url = long_url;
    url_info.created_at = get_current_timestamp();
    url_info.click_count = 0;
    url_info.creator = "guest";
    
    url_map_[short_code] = url_info;
    
    return short_code;
}

std::string URLShortener::resolve_url(const std::string& short_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = url_map_.find(short_code);
    if (it == url_map_.end()) {
        return "";
    }
    
    // Increment click count
    it->second.click_count++;
    
    return it->second.original_url;
}

bool URLShortener::delete_url(const std::string& short_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    return url_map_.erase(short_code) > 0;
}

ShortenedURL* URLShortener::get_url_info(const std::string& short_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = url_map_.find(short_code);
    if (it == url_map_.end()) {
        return nullptr;
    }
    
    return &it->second;
}

std::unordered_map<std::string, ShortenedURL> URLShortener::get_all_urls() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return url_map_;
}

int URLShortener::get_total_urls() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return url_map_.size();
}

int URLShortener::get_total_clicks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int total = 0;
    for (const auto& [code, url_info] : url_map_) {
        total += url_info.click_count;
    }
    return total;
}

std::string URLShortener::generate_short_code(int length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<> dist(0, chars.size() - 1);
    
    std::string code;
    for (int i = 0; i < length; ++i) {
        code += chars[dist(rng_)];
    }
    
    return code;
}

bool URLShortener::is_valid_url(const std::string& url) {
    if (url.empty() || url.length() < 10) {
        return false;
    }
    
    // Basic URL validation
    return (url.substr(0, 7) == "http://" || url.substr(0, 8) == "https://");
}

std::string URLShortener::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t_now), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

} // namespace services
