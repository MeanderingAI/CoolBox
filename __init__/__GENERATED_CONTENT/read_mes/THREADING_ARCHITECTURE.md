# Threading Architecture - MATLAB Platform Demo

## Overview
The MATLAB platform demo uses a **thread-per-request** model to handle concurrent HTTP requests without blocking.

## Main Server Threading Model

### Location: `demos/matlab_platform_demo.cpp` (Lines 35-65)

```cpp
void start() {
    running_ = true;
    
    // Create TCP socket
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // Bind to port
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    bind(server_fd_, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd_, 10);
    
    std::cout << "✓ MATLAB-Style App Launcher running on http://localhost:" << port_ << "\n";
    
    // MAIN EVENT LOOP - runs on main thread
    while (running_) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);
        
        // Accept blocks until client connects
        int client_fd = accept(server_fd_, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) continue;
        
        // SPAWN NEW THREAD FOR EACH REQUEST
        // This prevents blocking - each request is handled concurrently
        std::thread([this, client_fd]() {
            handle_request(client_fd);  // Process request
            close(client_fd);            // Close connection
        }).detach();                     // Detach so thread runs independently
    }
}
```

## Request Handling Flow

### 1. Main Thread (Event Loop)
- **Role**: Accept incoming connections
- **Blocking**: Yes - blocks on `accept()` until client connects
- **Responsibility**: Spawn worker threads

### 2. Worker Threads (One per request)
- **Role**: Handle individual HTTP request/response
- **Blocking**: Can block on I/O without affecting other requests
- **Lifecycle**: 
  1. Thread spawned by main loop
  2. `handle_request()` executes
  3. Socket closed
  4. Thread exits automatically (detached)

### 3. Request Logging (Lines 79-130)

```cpp
void handle_request(int client_fd) {
    // Read request
    char buffer[16384] = {0};
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    // Parse HTTP method and path
    size_t method_end = request.find(" ");
    std::string method = request.substr(0, method_end);
    size_t path_start = method_end + 1;
    size_t path_end = request.find(" ", path_start);
    std::string path = request.substr(path_start, path_end - path_start);
    
    // LOG INCOMING REQUEST with thread ID
    std::cout << "🔵 [" << std::this_thread::get_id() << "] " 
              << method << " " << path << std::endl;
    
    // Route to appropriate handler...
}
```

## Why Thread-Per-Request?

### Before (Synchronous - BLOCKING):
```
Client 1 connects → process request 1 (BLOCKS) → close
                    ⏰ All other clients wait here!
Client 2 connects → process request 2 (BLOCKS) → close
Client 3 connects → process request 3 (BLOCKS) → close
```

**Problem**: If `/app/frontends` takes 2 seconds to scan directories with `system()`, 
all other requests (images, CSS, etc.) are blocked. Page loads are stuck!

### After (Thread-Per-Request - CONCURRENT):
```
Client 1 connects → spawn thread 1 → process request 1 (in background)
Client 2 connects → spawn thread 2 → process request 2 (in background)
Client 3 connects → spawn thread 3 → process request 3 (in background)
                    ✅ All run concurrently!
```

**Benefit**: Multiple requests process simultaneously. Page loads are fast even if 
one request is slow (like directory scanning).

## Thread Safety Considerations

### Shared State
- `session_cache_` - Map accessed by multiple threads
- `html_cache_` - Map accessed by multiple threads
- `url_shortener_` - Service instance
- `system_monitor_` - Service instance
- `service_breaker_` - Service instance

### Current Status
⚠️ **No synchronization** - Maps are not thread-safe by default in C++

### Potential Issues
- Race conditions when reading/writing session cache
- Corruption if multiple threads modify html_cache_ simultaneously

### Recommended Fix (Not yet implemented)
```cpp
// Add mutexes for shared state
std::mutex session_mutex_;
std::mutex html_cache_mutex_;

// Lock when accessing shared maps
std::lock_guard<std::mutex> lock(session_mutex_);
session_cache_[key] = value;
```

## Service Manager Threading

### Location: `demos/matlab_platform_demo.cpp` (Lines 1980-2100)

The ServiceManager also uses threading for background operations:

```cpp
class ServiceManager {
private:
    std::thread output_reader_;  // Background thread for reading service outputs
    
    void start_all() {
        // Start output reading thread
        output_reader_ = std::thread(&ServiceManager::read_service_outputs, this);
        
        // Start services...
    }
    
    void read_service_outputs() {
        // Runs in background, continuously reads service logs
        while (running_) {
            // Poll service outputs...
        }
    }
};
```

## UI Thread

### Location: `demos/matlab_platform_demo.cpp` (Lines 2344-2491)

```cpp
int main() {
    // Start MATLAB-style UI in background thread
    std::thread ui_thread([&]() {
        ui.start();  // Runs HTTP server event loop
    });
    
    // Main thread continues with service manager UI
    // ...
    
    ui_thread.detach();  // UI runs independently
}
```

## Summary

### Threading Model: **Thread-Per-Request**
- ✅ Simple to implement
- ✅ Concurrent request handling
- ✅ No request blocking
- ⚠️ Unbounded thread creation (could be issue under heavy load)
- ⚠️ No thread-safe synchronization for shared state

### Alternative Approaches (for future):
1. **Thread Pool**: Pre-create fixed number of worker threads
2. **Async I/O**: Use event-driven model (epoll/kqueue)
3. **Coroutines**: C++20 co_await for lightweight concurrency

### Current Performance Characteristics:
- **Good**: Handles multiple page loads concurrently
- **Good**: Background operations don't block UI
- **Risk**: Under high load (100+ concurrent requests), could spawn too many threads
- **Risk**: Shared state access without locks could corrupt data

### Next Steps:
1. Add mutexes for session_cache_ and html_cache_
2. Consider thread pool for bounded resource usage
3. Add request rate limiting
