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

The platform includes a TUI-based service manager with **separate sections for backend and frontend services**:

```
⚙️  BACKEND SERVICES
┌────┬─────────────────────────┬──────┬──────────┬───────────────────────┐
│ ID │ SERVICE NAME            │ PORT │ STATUS   │ DESCRIPTION           │
├────┼─────────────────────────┼──────┼──────────┼───────────────────────┤
│  1 │ Frontend                │ 9000 │ ● RUN  │ MATLAB-Style Web UI   │
│  2 │ Metrics Backend         │ 9001 │ ○ STOP │ System metrics API    │
│  3 │ Account Service         │ 9002 │ ○ STOP │ User account mgmt     │
└────┴─────────────────────────┴──────┴──────────┴───────────────────────┘

🎨 FRONTEND SERVICES
┌────┬─────────────────────────┬──────┬──────────┬───────────────────────┐
│ ID │ SERVICE NAME            │ PORT │ STATUS   │ DESCRIPTION           │
├────┼─────────────────────────┼──────┼──────────┼───────────────────────┤
│  4 │ Account Frontend        │ 3000 │ ○ STOP │ Bun.js account UI     │
│  5 │ Metrics Frontend        │ 3001 │ ○ STOP │ Metrics dashboard     │
│  6 │ Admin Frontend          │ 3002 │ ○ STOP │ Admin panel           │
└────┴─────────────────────────┴──────┴──────────┴───────────────────────┘
```

### Backend Commands

- **`[1-3]`** - Start backend by ID
- **`s [1-3]`** - Stop backend by ID
- **`r [1-3]`** - Restart backend by ID
- **`log [1-3]`** - View backend logs

### Frontend Commands (NEW!)

- **`f [4-6]`** - Start frontend by ID (e.g., `f 4`)
- **`sf [4-6]`** - Stop frontend by ID (e.g., `sf 4`)
- **`rf [4-6]`** - Restart frontend by ID (e.g., `rf 4`)
- **`log [4-6]`** - View frontend logs

### General Commands

- **`open`** - Open main UI in browser
- **`refresh`** - Refresh display
- **`quit`** - Stop all and exit

## Services

### 1. Frontend (Port 9000)
- Main web UI
- MATLAB-style dashboard
- Authentication system
- Login: admin/admin123 or user/user123

### 2. Metrics Backend (Port 9001)

**Description**: Real-time system monitoring service that provides CPU, memory, disk, and network metrics.

**Technology**: C++ with raw socket HTTP server, integrated with SystemMonitor class

**Features**:
- Real-time system metrics collection
- JSON API responses
- CORS enabled for cross-origin requests
- Auto-updates every 2 seconds
- Low-overhead performance monitoring

**Endpoints**:
- `GET /api/metrics` - Current system metrics
  ```json
  {
    "cpu": 45.2,
    "memory": 62.8,
    "disk": 78.5,
    "network_rx": 1024.5,
    "network_tx": 512.3,
    "timestamp": 1735123456
  }
  ```
- `GET /health` - Health check (returns "OK")

### 3. Account Service (Port 9002)

**Description**: User account management and authentication service with session-based auth.

**Technology**: C++ with raw socket HTTP server, in-memory user storage

**Features**:
- User registration and login
- Session management with secure cookies
- Profile viewing and editing
- Password authentication (plain text - not production ready)
- Beautiful gradient UI (purple/blue theme)
- Form handling with URL decoding

**Routes**:
- `GET /` - Login page
- `POST /login` - Authentication handler
- `GET /signup` - Registration page
- `POST /signup` - Create new account
- `GET /profile` - View/edit profile (requires auth)
- `POST /profile/update` - Update profile information
- `GET /logout` - Destroy session and logout

**Data Structure**:
```cpp
struct UserAccount {
    std::string username;
    std::string password;
    std::string email;
    std::string full_name;
};
```

**Frontend Alternative**: 
A modern Bun.js frontend is available at [`/frontends/account-frontend`](../frontends/account-frontend) on port 3000. See below for setup.

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

# Terminal 4: Account Frontend (optional - Bun.js alternative UI)
cd frontends/account-frontend
bun install
bun dev
```

## Modern Frontend (Bun.js)

An alternative modern frontend for the account service is available:

**Location**: [`frontends/account-frontend/`](../frontends/account-frontend)  
**Port**: 3000  
**Technology**: Bun.js + HTML/CSS/JS

**Features**:
- 🎨 Modern dark theme with gradients
- 📱 Fully responsive mobile-friendly design
- 🔐 Session-based authentication
- ⚡ API proxy to account service (port 9002)
- 🚀 Fast - powered by Bun.js

**Quick Start**:
```bash
cd frontends/account-frontend
bun install
bun dev
```

Then open http://localhost:3000

**Pages**:
- `/` - Home page
- `/login.html` - Login
- `/signup.html` - Register
- `/profile.html` - Profile management

All API calls to `/api/*` are automatically proxied to the C++ account service on port 9002.

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
