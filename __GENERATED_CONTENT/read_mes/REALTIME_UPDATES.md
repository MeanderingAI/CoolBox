# Service Manager - Real-Time Updates Implementation

## Current Implementation

**HTTP Polling** (1-second intervals)
- Frontend fetches `/api/service/output?id=X` every 1 second
- Simple, reliable, no special server-side code needed
- Works with any HTTP proxy/load balancer
- Sufficient for most monitoring use cases

## Why Console Might Appear Empty

1. **Service not selected**: Click on the service row to select it (row turns purple)
2. **No output yet**: Service hasn't produced output
3. **Browser cache**: Hard refresh (Ctrl+Shift+R / Cmd+Shift+R)
4. **JavaScript error**: Check browser console (F12) for errors

## Troubleshooting Steps

```javascript
// In browser console (F12):
// 1. Check if service is selected
console.log('Selected service ID:', selectedServiceId);

// 2. Manually fetch output
fetch('/api/service/output?id=3')
  .then(r => r.json())
  .then(data => console.log('Output:', data));

// 3. Check for errors
window.onerror = (msg, url, line) => console.error('JS Error:', msg);
```

## For WebSocket/SSE Implementation (Future)

### Server-Sent Events (Simpler)
```cpp
// Add SSE endpoint
if (path == "/api/service/stream" && request.find("GET") != std::string::npos) {
    response = "HTTP/1.1 200 OK\r\n"
               "Content-Type: text/event-stream\r\n"
               "Cache-Control: no-cache\r\n"
               "Connection: keep-alive\r\n\r\n";
    write(client_fd, response.c_str(), response.length());
    
    // Keep connection open and stream updates
    while (running) {
        auto lines = service_manager_->get_service_output(service_id);
        std::string data = "data: " + json_encode(lines) + "\n\n";
        write(client_fd, data.c_str(), data.length());
        sleep(1);
    }
}
```

```javascript
// Frontend
const eventSource = new EventSource('/api/service/stream?id=3');
eventSource.onmessage = (event) => {
    const data = JSON.parse(event.data);
    updateConsole(data.lines);
};
```

### WebSocket (More Complex)
Requires:
- WebSocket handshake (HTTP Upgrade)
- Frame encoding/decoding (RFC 6455)
- Ping/pong heartbeat
- Binary/text frame support
- Close handshake

**Estimated implementation**: ~800 lines of code

## SSL/TLS Support

### Current Options

1. **Use nginx reverse proxy** (Recommended - see SSL_SETUP.md)
   - Production-ready
   - Handles SSL termination
   - Load balancing
   - Rate limiting

2. **Use Caddy** (Easiest)
   - Automatic HTTPS
   - Auto-renewing certificates
   - Simple configuration

3. **Native OpenSSL** (Complex)
   - Requires OpenSSL library
   - ~500 lines of code
   - Certificate management
   - Error handling

## Performance Considerations

### Current (HTTP Polling @ 1s)
- **Bandwidth**: ~1-2 KB/s per client
- **Latency**: Up to 1 second
- **Server load**: Minimal (simple HTTP requests)
- **Scalability**: Good (stateless)

### WebSocket/SSE
- **Bandwidth**: More efficient for frequent updates
- **Latency**: Real-time (<100ms)
- **Server load**: Higher (persistent connections)
- **Scalability**: Limited by open connections

## Recommended Setup for Production

```nginx
# nginx with SSL
upstream backend {
    server 127.0.0.1:9003;
}

server {
    listen 443 ssl http2;
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;
    
    location / {
        proxy_pass http://backend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
    
    # Future WebSocket support
    location /ws {
        proxy_pass http://backend;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
    }
}
```

## Current Status

✅ **Working**: HTTP polling at 1s intervals  
✅ **Working**: Console output capture  
✅ **Working**: Auto-refresh when service selected  
✅ **Working**: Hot-reload for HTML files  
✅ **Working**: File watcher for service source files  

⏳ **Future**: WebSocket/SSE for real-time  
⏳ **Future**: Native SSL support  
⏳ **Future**: Binary protocol for efficiency  

## Quick Fix for Empty Console

If console appears empty but terminal shows output:

1. **Select the service**: Click on the row (should turn purple)
2. **Check browser console**: F12 → Console tab → Look for errors
3. **Verify endpoint**: F12 → Network tab → Look for `/api/service/output?id=3`
4. **Check response**: Click on the request → Response tab → Should show JSON with lines
5. **Clear browser cache**: Ctrl+Shift+Del → Clear cache

If still empty:
```bash
# Check server logs
tail -f terminal_output.log

# Test endpoint manually
curl http://localhost:9003/api/service/output?id=3

# Should return: {"lines":["...", "..."]}
```
