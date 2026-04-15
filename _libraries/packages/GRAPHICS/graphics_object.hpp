#ifndef COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_GRAPHICS_OBJECT_HPP
#define COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_GRAPHICS_OBJECT_HPP

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace graphics {

class GraphicsObject {
public:
    virtual ~GraphicsObject() = default;

    virtual std::string graphics_object_kind() const = 0;
    virtual std::string graphics_object_name() const = 0;
};

class GraphicsObjectRegistry {
public:
    using Factory = std::function<std::unique_ptr<GraphicsObject>()>;

    bool register_factory(std::string key, Factory factory) {
        if (key.empty() || !factory) {
            return false;
        }
        return factories_.emplace(std::move(key), std::move(factory)).second;
    }

    template <typename T>
    bool register_type(std::string key) {
        return register_factory(std::move(key), []() {
            return std::make_unique<T>();
        });
    }

    bool contains(const std::string& key) const {
        return factories_.find(key) != factories_.end();
    }

    std::unique_ptr<GraphicsObject> create(const std::string& key) const {
        const auto it = factories_.find(key);
        if (it == factories_.end()) {
            throw std::invalid_argument("Unknown graphics object factory key: " + key);
        }
        return it->second();
    }

    std::vector<std::string> registered_keys() const {
        std::vector<std::string> keys;
        keys.reserve(factories_.size());
        for (const auto& entry : factories_) {
            keys.push_back(entry.first);
        }
        return keys;
    }

private:
    std::unordered_map<std::string, Factory> factories_;
};

} // namespace graphics

#endif  // COOLBOX__LIBRARIES_PACKAGES_GRAPHICS_GRAPHICS_OBJECT_HPP