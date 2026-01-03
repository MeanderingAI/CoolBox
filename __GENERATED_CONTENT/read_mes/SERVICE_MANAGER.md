# Service Manager (Hot-Reload Platform)

## Quick Start

### Build and Run
```bash
make service           # Build and run in one command
```

### Individual Commands
```bash
make build-service     # Build only
make run-service       # Run (assumes already built)
make rebuild-service   # Clean build and run
make kill-service      # Stop all instances
```

## Features

- **Hot-reload** for HTML files - Changes appear instantly
- **Auto-rebuild** on C++ changes - Services recompile automatically  
- **Service monitoring** - Real-time status and console output
- **Request logging** - Track all HTTP requests with thread IDs
- **CSV export** - Download logs and console output
- **nginx routing** - Configure reverse proxy settings
- **Port management** - Auto-detect conflicts and reassign

## Access URLs

- **Service Manager**: http://localhost:9003/app/manager
- **Main Dashboard**: http://localhost:9003

## Manual Commands (if needed)

```bash
# Build manually
cmake --build build --target service_manager -j8

# Run manually on custom port
build/demos/service_manager 8080

# Kill existing instances
pkill -f service_manager
```

## Service Architecture

### Backend Services (C++)
- **Metrics Backend** (Port 9001) - `/services/metrics_backend_service.cpp`
- **Account Service** (Port 9002) - `/services/account_service.cpp`
- **Proxy Service** (Port 8080) - `/services/proxy_service.cpp`

### Frontend Services
- **Cache UI** (Port 9004)
- **Web Frontends** (Port 9005)
- **Frontend UI** (Port 9006)

## Watchdog (Auto-Rebuild)

The watchdog monitors all `.cpp`, `.h`, and `.hpp` files in the `/services/` directory:

- **Check interval**: 2 seconds
- **Auto-rebuild**: When source files change
- **Auto-restart**: If service was running before rebuild
- **Statistics**: Tracks checks, rebuilds, and restarts

## Tips

- Use `make service` for development - it's the fastest way to start
- The server remembers which services were running between restarts
- Console output is preserved even when services stop/restart
- Hot-reload works for all files in `demos/resources/html/`
- Build information shows shared libraries and dependencies

## Troubleshooting

### Port already in use
The service manager will detect port conflicts and offer to:
1. Kill the conflicting process
2. Let you change the port manually
3. Auto-assign a free port

### Service won't start
Check the console tab for detailed error messages including:
- Missing executables
- Permission issues  
- Port conflicts
- Build failures

### Build errors
```bash
make rebuild-service    # Clean and rebuild
```

### Multiple instances running
```bash
make kill-service       # Kill all instances
```
