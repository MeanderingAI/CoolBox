// Example: How to use library metadata in your source files
// Add this to any .cpp file in your library (e.g., src/cache/cache_server.cpp)

#include "lib_metadata.h"

// Embed full metadata
LIBRARY_METADATA(
    cache_server,
    "Cache Server",
    "1.0.0",
    "High-performance in-memory caching system with LRU eviction and TTL support",
    "ToolBox Team"
)

// Or just use simple documentation
// LIBRARY_DOC("Cache server with LRU and TTL support")

// You can also document individual functions
FUNCTION_DOC(cache_get, "Retrieves a value from cache by key. Returns nullptr if not found.")
FUNCTION_DOC(cache_set, "Stores a key-value pair in cache with optional TTL.")
FUNCTION_DOC(cache_clear, "Clears all entries from the cache.")

// Your actual implementation
extern "C" {
    void* cache_get(const char* key) {
        // implementation
        return nullptr;
    }
    
    void cache_set(const char* key, void* value, int ttl) {
        // implementation
    }
    
    void cache_clear() {
        // implementation
    }
}

int main() {
    // Minimal entry point for linker
    return 0;
}
