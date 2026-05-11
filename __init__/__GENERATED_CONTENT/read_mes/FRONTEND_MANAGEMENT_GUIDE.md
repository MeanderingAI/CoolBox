# Frontend Management in MATLAB Platform - Quick Guide

## Overview

The MATLAB Platform Service Manager now includes a dedicated **Frontend Services** section for managing Bun.js and other web frontends!

## What's New

### 🎨 Separate Frontend Section

The TUI now displays services in two distinct sections:

1. **⚙️  BACKEND SERVICES** (IDs 1-3)
   - Frontend (port 9000) - MATLAB-Style Web UI
   - Metrics Backend (port 9001) - System metrics API
   - Account Service (port 9002) - User account management

2. **🎨 FRONTEND SERVICES** (IDs 4-6)
   - Account Frontend (port 3000) - Bun.js account UI
   - Metrics Frontend (port 3001) - Metrics dashboard (TBD)
   - Admin Frontend (port 3002) - Admin panel (TBD)

### ⚡ New Commands

Frontend-specific commands use the `f` prefix:

| Command | Action | Example |
|---------|--------|---------|
| `f [4-6]` | Start frontend | `f 4` starts Account Frontend |
| `sf [4-6]` | Stop frontend | `sf 4` stops Account Frontend |
| `rf [4-6]` | Restart frontend | `rf 4` restarts Account Frontend |
| `log [4-6]` | View logs | `log 4` shows Account Frontend logs |

### 📊 Real-Time Status

Each service shows:
- **●** Green = Running
- **○** Red = Stopped
- Port number
- Description

## Usage Example

```bash
# 1. Start the service manager
cd build/demos
./matlab_platform_demo

# 2. Start backend services
Command: 3
[Starts Account Service on port 9002]

# 3. Start frontend
Command: f 4
[Starts Account Frontend on port 3000]

# 4. View logs
Command: log 4
[Shows real-time output from Bun.js server]

# 5. Open in browser
Command: open
[Opens http://localhost:3000 in browser]

# 6. Restart frontend
Command: rf 4
[Stops and restarts Account Frontend]

# 7. Stop frontend
Command: sf 4
[Stops Account Frontend]

# 8. Exit
Command: quit
```

## Prerequisites

### Install Bun.js

Frontends require Bun to be installed:

```bash
# Install Bun
curl -fsSL https://bun.sh/install | bash

# Reload shell
source ~/.zshrc

# Verify
bun --version
```

### Install Frontend Dependencies

For the Account Frontend:

```bash
cd frontends/account-frontend
bun install
```

## Architecture

```
┌─────────────────────────────────────┐
│   Service Manager (matlab_platform) │
│         Port 9000 (C++)             │
└────────────┬────────────────────────┘
             │
       ┌─────┴─────┐
       ↓           ↓
┌─────────────┐  ┌─────────────┐
│  Backend    │  │  Frontends  │
│  Services   │  │  (Bun.js)   │
└─────────────┘  └─────────────┘
       ↓                ↓
  Port 9001-9002   Port 3000-3002
```

## Features

### 🚀 Easy Lifecycle Management
- Start, stop, restart with simple commands
- No need to remember complex shell commands
- Automatic process management

### 📝 Log Viewing
- Real-time log streaming
- Last 100 lines cached
- Automatic scroll and formatting

### 🔄 Service Dependencies
- Backend services start independently
- Frontends proxy to backends
- Clear port allocation

### 💡 Visual Feedback
- Color-coded status indicators
- Service descriptions
- Port information
- Execution feedback

## Tips

1. **Start Backend First**: Always start the backend service (e.g., service 3) before starting its frontend (e.g., service 4)

2. **Check Logs**: If a frontend fails to start, use `log 4` to see error messages

3. **Port Conflicts**: If you get "address already in use", check with:
   ```bash
   lsof -ti:3000
   kill -9 $(lsof -ti:3000)
   ```

4. **Browser Cache**: Clear browser cache if you see old content after restart

5. **Development**: Edit frontend files in `frontends/account-frontend/public/` and restart with `rf 4`

## Troubleshooting

### Frontend won't start

**Symptom**: `✗ Failed to start frontend 4`

**Solutions**:
1. Check Bun is installed: `which bun`
2. Install dependencies: `cd frontends/account-frontend && bun install`
3. Check backend is running: `curl http://localhost:9002/api/health`
4. View logs: `log 4`

### Can't access frontend

**Symptom**: Browser shows "connection refused"

**Solutions**:
1. Check service is running in TUI (● = green)
2. Check port: `lsof -ti:3000`
3. Check logs: `log 4`
4. Try restarting: `rf 4`

### Frontend shows errors

**Symptom**: Logs show errors or exceptions

**Solutions**:
1. Check backend connectivity: `curl http://localhost:9002/api/health`
2. Verify API endpoints match in `server.ts`
3. Check console in browser DevTools
4. Clear browser cache

## Adding New Frontends

To add a new frontend (e.g., metrics dashboard):

1. **Create Directory**
   ```bash
   cd frontends
   cp -r account-frontend metrics-frontend
   cd metrics-frontend
   ```

2. **Update Configuration**
   - Edit `server.ts`: Change port to 3001
   - Edit `package.json`: Update name
   - Update API proxy endpoint to port 9001

3. **Update Service Manager**
   Edit `demos/matlab_platform_demo.cpp`:
   ```cpp
   services_[5] = {"Metrics Frontend", 
                   "cd ./frontends/metrics-frontend && bun run server.ts", 
                   3001, -1, false, "Metrics dashboard", {}, -1};
   ```

4. **Rebuild**
   ```bash
   cd ../../build
   make matlab_platform_demo
   ```

5. **Test**
   ```bash
   ./demos/matlab_platform_demo
   # Then: f 5
   ```

## Next Steps

- [ ] Implement Metrics Frontend (ID 5)
- [ ] Implement Admin Frontend (ID 6)
- [ ] Add WebSocket support for real-time updates
- [ ] Add hot-reload for development
- [ ] Add health checks for frontends
- [ ] Add auto-restart on crash

## Documentation

- **Frontends Overview**: `frontends/README.md`
- **Account Frontend**: `frontends/account-frontend/README.md`
- **Services Overview**: `services/README.md`
- **Demo Script**: `./demo_frontend_management.sh`

## Summary

The new frontend management system provides:

✅ **Unified Control** - Manage backends and frontends from one TUI  
✅ **Simple Commands** - Intuitive `f`, `sf`, `rf` commands  
✅ **Real-Time Logs** - See what's happening instantly  
✅ **Clean Separation** - Backend vs Frontend sections  
✅ **Extensible** - Easy to add new frontends  
✅ **Professional** - Production-ready service management  

**Start using it:**
```bash
cd build/demos
./matlab_platform_demo
```

Enjoy your new frontend management capabilities! 🎉
