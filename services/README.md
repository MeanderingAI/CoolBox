# ToolBox Services

Backend services for the MATLAB-Style Platform.

## Quick Start

```bash
# Build all services
cd services/build
cmake ..
make -j8

# Or run the platform manager (recommended)
cd ../..
./build/demos/matlab_platform_demo
```

## Interactive Service Manager

The platform includes a TUI-based service manager:

```
┌────┬─────────────────────────┬──────┬──────────┬───────────────────────┐
│ ID │ SERVICE NAME            │ PORT │ STATUS   │ DESCRIPTION           │
├────┼─────────────────────────┼──────┼──────────┼───────────────────────┤
│  1 │ Frontend                │ 9000 │ ● RUN  │ MATLAB-Style Web UI   │
│  2 │ Metrics Backend         │ 9001 │ ○ STOP │ System metrics API    │
│  3 │ Account Service         │ 9002 │ ○ STOP │ User account mgmt     │
└────┴─────────────────────────┴──────┴──────────┴───────────────────────┘
```

### Commands

- **`2`** - Start service #2 (Metrics Backend)
- **`3`** - Start service #3 (Account Service)
- **`s 2`** - Stop service #2
- **`r 2`** - Restart service #2
- **`log 2`** - View service #2 output logs
- **`open`** - Open UI in browser
- **`refresh`** - Refresh display
- **`quit`** - Stop all and exit

## Services

### 1. Frontend (Port 9000)
- Main web UI
- MATLAB-style dashboard
- Authentication system
- Login: admin/admin123 or user/user123

### 2. Metrics Backend (Port 9001)
- System metrics API
- Endpoints:
  - `GET /api/metrics` - Current system metrics (CPU, Memory, Disk, Network)
  - `GET /health` - Health check
- Auto-updates every 2 seconds
- CORS enabled

### 3. Account Service (Port 9002)
- User account management
- Session-based authentication
- Routes:
  - `GET /` - Login page
  - `POST /login` - Login handler
  - `GET /signup` - Signup page
  - `POST /signup` - Create account
  - `GET /profile` - View/edit profile
  - `POST /profile/update` - Update profile
  - `GET /logout` - Logout

## Manual Service Control

If you prefer running services manually:

```bash
# Terminal 1: Frontend
./build/demos/matlab_platform_demo

# Terminal 2: Metrics Backend
cd services/build
./metrics_backend_service 9001

# Terminal 3: Account Service
cd services/build
./account_service 9002
```

## Features

✅ **Interactive TUI** - Number-based service control  
✅ **Live Logs** - View real-time service output  
✅ **Auto-restart** - Restart crashed services  
✅ **Status Monitoring** - Live service status indicators  
✅ **Output Capture** - Last 100 lines of each service  

## Architecture

```
┌──────────────────────────────────────────┐
│  matlab_platform_demo (Service Manager) │
│          Port 9000 (Frontend)            │
└──────────────┬───────────────────────────┘
               │
    ┌──────────┴──────────┐
    │                     │
┌───▼────┐          ┌─────▼────┐
│Metrics │          │ Account  │
│Backend │          │ Service  │
│Port9001│          │ Port9002 │
└────────┘          └──────────┘
```

## Development

- **Hot-reload**: HTML files in `demos/resources/html/` reload automatically
- **Logs**: Use `log <id>` command to view service output
- **Debug**: Services run as child processes with captured output

## Troubleshooting

**Service won't start:**
- Check if port is already in use: `lsof -i :9001`
- View logs: Type `log 2` in the manager

**Frontend stuck at "connecting":**
- Ensure Metrics Backend is running (service #2)
- Check logs: `log 2`

**Can't stop service:**
- Use `s <id>` to gracefully stop
- Or restart: `r <id>`
