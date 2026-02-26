
#include "services/url_shortener/url_shortener.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <fstream>
#include <filesystem>

// Library metadata
extern "C" {
    __attribute__((weak, used))
    const char* get_library_name_url_shortener() { return "url_shortener"; }
    
    __attribute__((weak, used))
    const char* get_library_version_url_shortener() { return "1.0.0"; }
    
    __attribute__((weak, used))
    const char* get_library_description_url_shortener() { return "URL shortening service with custom aliases, click tracking, and analytics"; }
    
    __attribute__((weak, used))
    const char* get_library_author_url_shortener() { return "ToolBox Team"; }
}

namespace services {

namespace fs = std::filesystem;

static const std::string kLogDir = "site_content";
static const std::string kLogFile = "site_content/urls.log";

URLShortener::URLShortener() : rng_(std::random_device{}()) {
    // Load from log if exists
    if (fs::exists(kLogFile)) {
        std::ifstream in(kLogFile);
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream iss(line);
            ShortenedURL url;
            if (std::getline(iss, url.short_code, ',') &&
                std::getline(iss, url.original_url, ',') &&
                std::getline(iss, url.created_at, ',') &&
                iss >> url.click_count && iss.get() &&
                std::getline(iss, url.creator)) {
                url_map_[url.short_code] = url;
            }
        }
    }
}

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
    // Log to file
    fs::create_directories(kLogDir);
    std::ofstream out(kLogFile, std::ios::app);
    out << short_code << "," << long_url << "," << url_info.created_at << "," << url_info.click_count << "," << url_info.creator << "\n";
    return short_code;
}

std::string URLShortener::resolve_url(const std::string& short_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = url_map_.find(short_code);
    if (it != url_map_.end()) {
        it->second.click_count++;
        // Update log (rewrite all)
        std::ofstream out(kLogFile, std::ios::trunc);
        for (const auto& [code, url] : url_map_) {
            out << url.short_code << "," << url.original_url << "," << url.created_at << "," << url.click_count << "," << url.creator << "\n";
        }
        return it->second.original_url;
    }
    return "";
}

bool URLShortener::delete_url(const std::string& short_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = url_map_.find(short_code);
    if (it != url_map_.end()) {
        url_map_.erase(it);
        // Update log (rewrite all)
        std::ofstream out(kLogFile, std::ios::trunc);
        for (const auto& [code, url] : url_map_) {
            out << url.short_code << "," << url.original_url << "," << url.created_at << "," << url.click_count << "," << url.creator << "\n";
        }
        return true;
    }
    return false;
}

ShortenedURL* URLShortener::get_url_info(const std::string& short_code) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = url_map_.find(short_code);
    if (it != url_map_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::unordered_map<std::string, ShortenedURL> URLShortener::get_all_urls() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return url_map_;
}

int URLShortener::get_total_urls() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(url_map_.size());
}

int URLShortener::get_total_clicks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int total = 0;
    for (const auto& [code, url] : url_map_) {
        total += url.click_count;
    }
    return total;
}

std::string URLShortener::generate_short_code(int length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);
    std::string code;
    for (int i = 0; i < length; ++i) {
        code += charset[dist(rng_)];
    }
    return code;
}

bool URLShortener::is_valid_url(const std::string& url) {
    // Simple check for http(s)://
    return url.find("http://") == 0 || url.find("https://") == 0;
}

std::string URLShortener::get_current_timestamp() {
    std::time_t now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    return buf;
}


// ...existing code continues...
} // namespace services
