# Proxy Configuration Directory

This directory contains configuration files for the Proxy Service (nginx-clone).

## Directory Structure

```
proxy_configs/
├── default.conf          # Default proxy configuration
├── production.conf       # Production environment config (create as needed)
├── development.conf      # Development environment config (create as needed)
└── README.md            # This file
```

## Configuration File Format

The proxy service uses INI-style configuration files with the following sections:

### [proxy] Section
Main proxy server settings:
- `listen_port`: Port to listen on (default: 8080)
- `enable_ssl`: Enable SSL/TLS (true/false)
- `ssl_cert_path`: Path to SSL certificate file
- `ssl_key_path`: Path to SSL private key file
- `worker_threads`: Number of worker threads (default: 4)
- `max_connections`: Maximum concurrent connections (default: 1000)
- `keepalive_timeout`: Keep-alive timeout in seconds (default: 65)
- `upstream_connect_timeout`: Timeout for connecting to upstream (default: 5)
- `upstream_read_timeout`: Timeout for reading from upstream (default: 60)
- `enable_logging`: Enable request logging (true/false)
- `max_cache_size`: Maximum cache size in bytes (default: 100MB)

### [route.*] Sections
Define routing rules and upstream servers:
- `path_prefix`: URL path prefix to match (e.g., "/api", "/static", "/")
- `enable_cache`: Enable caching for this route (true/false)
- `cache_ttl_seconds`: Cache time-to-live in seconds
- `balance_method`: Load balancing method
  - `round_robin`: Distribute requests evenly across upstreams
  - `least_conn`: Send to upstream with fewest active connections
  - `weighted`: Weighted distribution based on upstream weights
- `upstreams`: Comma-separated list of upstream servers
  - Format: `host:port:weight`
  - Example: `localhost:9001:2,localhost:9002:1`
  - Weight determines relative load (higher = more traffic)

## Usage

### Start proxy with specific config:
```bash
./services/build/proxy_service --config proxy_configs/default.conf
```

### Start proxy with default settings:
```bash
./services/build/proxy_service 8080
```

## Load Balancing Strategies

### Round Robin
Distributes requests evenly across all healthy upstream servers. Simple and effective for most use cases.

```ini
[route.example]
balance_method = round_robin
upstreams = localhost:9001:1,localhost:9002:1,localhost:9003:1
```

### Least Connections
Routes requests to the upstream server with the fewest active connections. Best for long-running requests or WebSocket connections.

```ini
[route.api]
balance_method = least_conn
upstreams = localhost:9001:1,localhost:9002:1
```

### Weighted Distribution
Distributes requests based on server capacity/weight. Useful when upstream servers have different capabilities.

```ini
[route.heavy]
balance_method = weighted
upstreams = powerful-server:9001:3,normal-server:9002:1
# powerful-server will receive ~75% of traffic, normal-server ~25%
```

## Caching

The proxy service includes an in-memory cache with TTL (Time To Live) support:

- Enable caching per route with `enable_cache = true`
- Set cache duration with `cache_ttl_seconds`
- Total cache size limited by `max_cache_size` in [proxy] section
- Cache automatically evicts expired entries
- Only GET requests are cached
- Only 200 OK responses are cached

### Example: Cache static assets for 5 minutes
```ini
[route.static]
path_prefix = /static
enable_cache = true
cache_ttl_seconds = 300
upstreams = localhost:9003:1
```

### Example: Don't cache dynamic API responses
```ini
[route.api]
path_prefix = /api
enable_cache = false
upstreams = localhost:9001:1
```

## SSL/TLS Configuration

To enable HTTPS:

1. Obtain SSL certificate and private key
2. Update configuration:
```ini
[proxy]
listen_port = 443
enable_ssl = true
ssl_cert_path = /path/to/certificate.pem
ssl_key_path = /path/to/private-key.pem
```

### Generate self-signed certificate for testing:
```bash
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes
```

Then update config:
```ini
ssl_cert_path = proxy_configs/cert.pem
ssl_key_path = proxy_configs/key.pem
```

## Health Checks

The proxy service automatically performs health checks on upstream servers:
- Checks every 10 seconds
- TCP connection test to upstream port
- Marks unhealthy after 3 consecutive failures (configurable via `max_fails`)
- Automatically routes around unhealthy upstreams
- Re-enables upstream when health check succeeds

## Example Configurations

### Simple reverse proxy
```ini
[proxy]
listen_port = 8080
enable_logging = true

[route.default]
path_prefix = /
enable_cache = false
balance_method = round_robin
upstreams = localhost:3000:1
```

### Multi-service architecture
```ini
[proxy]
listen_port = 80
worker_threads = 8

[route.api]
path_prefix = /api/v1
enable_cache = false
balance_method = least_conn
upstreams = api-server-1:9001:1,api-server-2:9002:1

[route.static]
path_prefix = /static
enable_cache = true
cache_ttl_seconds = 3600
balance_method = round_robin
upstreams = cdn-server:9003:1

[route.frontend]
path_prefix = /
enable_cache = true
cache_ttl_seconds = 60
balance_method = round_robin
upstreams = web-server:3000:1
```

## Monitoring

The proxy service logs:
- All incoming requests (when `enable_logging = true`)
- Upstream health check results
- Cache hit/miss statistics
- Connection errors

Press Ctrl+C in the proxy service to see statistics summary.

## Troubleshooting

### "Failed to bind to port"
- Port is already in use
- Solution: Change `listen_port` or kill process using the port

### "Failed to connect to upstream server"
- Upstream service is not running
- Incorrect host/port in upstreams
- Firewall blocking connection
- Solution: Verify upstream service is running and accessible

### "All upstream servers are down"
- All upstreams failed health checks
- Solution: Check upstream services, review logs

### High cache miss rate
- Cache TTL too short
- Requests not cacheable (non-GET or non-200 responses)
- Solution: Increase `cache_ttl_seconds` or verify request methods
