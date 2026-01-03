#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <random>
#include <chrono>

namespace services {

struct ShortenedURL {
    std::string short_code;
    std::string original_url;
    std::string created_at;
    int click_count;
    std::string creator;
};

class URLShortener {
public:
    URLShortener();
    
    // Core operations
    std::string shorten_url(const std::string& long_url, const std::string& custom_code = "");
    std::string resolve_url(const std::string& short_code);
    bool delete_url(const std::string& short_code);
    
    // Statistics
    ShortenedURL* get_url_info(const std::string& short_code);
    std::unordered_map<std::string, ShortenedURL> get_all_urls() const;
    int get_total_urls() const;
    int get_total_clicks() const;
    
private:
    std::string generate_short_code(int length = 6);
    bool is_valid_url(const std::string& url);
    std::string get_current_timestamp();
    
    std::unordered_map<std::string, ShortenedURL> url_map_;
    mutable std::mutex mutex_;
    std::mt19937 rng_;
};

} // namespace services
