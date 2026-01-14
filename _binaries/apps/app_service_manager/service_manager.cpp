// Service Manager - Shared Library Build Management
//
// A focused tool for managing and rebuilding shared C++ libraries.
//
// Features:
// - Displays all .so/.dylib files in build/libraries/src/
// - Shows library details (size, last modified, make target)
// - Individual rebuild buttons for each library
// - Real-time library scanning
//
// Usage: ./service_manager [port]
// Default port: 9004
//
// Access at: http://localhost:9004

// Standard library headers
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <dirent.h>
#include <fstream>
#include <memory>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <dlfcn.h>

// Project headers
#include "networking/rest_api_DEPRECATED/http2_servlet.h"
#include "dataformats/json/json.h"
#include "networking/http/request_response.h"
#include "utils.hpp"
#include "request_handlers.h"
#include "server_constants.hpp"

#include "binary_info.hpp"
#include "shared_library.hpp"
#include "advanced_logging/advanced_logging.h"



int main(int argc, char* argv[]) {
    return 0;
}
