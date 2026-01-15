# SSL/HTTPS Setup for Service Manager

## Option 1: Using nginx as Reverse Proxy (Recommended)

### 1. Install nginx
```bash
# macOS
brew install nginx

# Ubuntu/Debian
sudo apt-get install nginx

# CentOS/RHEL
sudo yum install nginx
```

### 2. Generate SSL Certificate
```bash
# Self-signed certificate for development
sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout /etc/ssl/private/service-manager.key \
  -out /etc/ssl/certs/service-manager.crt

# Or use Let's Encrypt for production
sudo certbot --nginx -d yourdomain.com
```

### 3. Configure nginx
Create `/etc/nginx/sites-available/service-manager`:

```nginx
# HTTP to HTTPS redirect
server {
    listen 80;
    server_name localhost;
    return 301 https://$host$request_uri;
}

# HTTPS server
server {
    listen 443 ssl http2;
    server_name localhost;

    # SSL certificates
    ssl_certificate /etc/ssl/certs/service-manager.crt;
    ssl_certificate_key /etc/ssl/private/service-manager.key;

    # SSL configuration
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;
    ssl_prefer_server_ciphers on;

    # Proxy to service manager
    location / {
        proxy_pass http://localhost:9003;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_cache_bypass $http_upgrade;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # WebSocket support (for future implementation)
    location /ws/ {
        proxy_pass http://localhost:9003;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
        proxy_set_header Host $host;
        proxy_read_timeout 86400;
    }
}
```

### 4. Enable and restart nginx
```bash
# Link configuration
sudo ln -s /etc/nginx/sites-available/service-manager /etc/nginx/sites-enabled/

# Test configuration
sudo nginx -t

# Restart nginx
sudo systemctl restart nginx
```

### 5. Access securely
```
https://localhost/app/manager
```

## Option 2: Using Caddy (Automatic HTTPS)

### 1. Install Caddy
```bash
# macOS
brew install caddy

# Ubuntu/Debian
sudo apt install -y debian-keyring debian-archive-keyring apt-transport-https
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/gpg.key' | sudo gpg --dearmor -o /usr/share/keyrings/caddy-stable-archive-keyring.gpg
curl -1sLf 'https://dl.cloudsmith.io/public/caddy/stable/debian.deb.txt' | sudo tee /etc/apt/sources.list.d/caddy-stable.list
sudo apt update
sudo apt install caddy
```

### 2. Create Caddyfile
```caddy
localhost {
    reverse_proxy localhost:9003
}
```

### 3. Run Caddy
```bash
sudo caddy run --config Caddyfile
```

Caddy automatically handles HTTPS with self-signed certificates for localhost or Let's Encrypt for production domains.

## Option 3: Native SSL (Future Enhancement)

To add native SSL support to the C++ server, you would need to:

1. **Install OpenSSL development libraries**
```bash
# macOS
brew install openssl

# Ubuntu/Debian  
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel
```

2. **Link OpenSSL in CMakeLists.txt**
```cmake
find_package(OpenSSL REQUIRED)
target_link_libraries(matlab_platform_demo OpenSSL::SSL OpenSSL::Crypto)
```

3. **Implement SSL wrapper** around socket operations:
- `SSL_CTX_new()` for SSL context
- `SSL_new()` for SSL connection
- `SSL_accept()` for handshake
- `SSL_read()`/`SSL_write()` instead of `read()`/`write()`

This would add ~500 lines of code for proper SSL handling, certificate loading, and error management.

## WebSocket Implementation (Future)

For real-time console updates via WebSocket:

1. **WebSocket handshake** (HTTP Upgrade request)
2. **Frame encoding/decoding** per RFC 6455
3. **Ping/pong heartbeat**
4. **Connection management**

Current implementation uses HTTP polling (1s interval) which is sufficient for most use cases.

## Security Best Practices

1. **Always use strong passwords** for the authentication system
2. **Keep SSL certificates up to date**
3. **Use environment variables** for sensitive configuration
4. **Enable CORS restrictions** in production
5. **Use firewall rules** to limit access
6. **Keep dependencies updated**

## Production Deployment

For production use:

```bash
# 1. Use systemd service
sudo systemctl enable service-manager
sudo systemctl start service-manager

# 2. Use reverse proxy (nginx/Caddy)
# 3. Use Let's Encrypt for SSL
# 4. Enable firewall
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw enable

# 5. Monitor logs
journalctl -u service-manager -f
```

## Troubleshooting

**Certificate errors:**
```bash
# Check certificate
openssl x509 -in /etc/ssl/certs/service-manager.crt -text -noout

# Test SSL connection
openssl s_client -connect localhost:443
```

**nginx errors:**
```bash
# Check logs
sudo tail -f /var/log/nginx/error.log

# Test config
sudo nginx -t
```

**Port conflicts:**
```bash
# Check what's using port 443
sudo lsof -i:443

# Kill process if needed
sudo kill -9 <PID>
```
