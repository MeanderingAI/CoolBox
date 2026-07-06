#include "cli_tools.hpp"
#include "database.h"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#if !defined(_WIN32)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace {

constexpr int k_default_port = 55432;
constexpr const char* k_end_marker = "<<END>>";

void print_result(const ml::sql::ResultSet& result) {
    if (!result.columns.empty()) {
        std::cout << "columns:";
        for (const auto& col : result.columns) {
            std::cout << ' ' << col;
        }
        std::cout << "\n";
    }

    for (const auto& row : result.rows) {
        bool first = true;
        for (const auto& col : result.columns) {
            if (!first) {
                std::cout << " | ";
            }
            const auto it = row.find(col);
            std::cout << col << '=' << (it == row.end() ? "" : it->second);
            first = false;
        }
        std::cout << "\n";
    }

    std::cout << "affected_rows=" << result.affected_rows
              << " last_insert_id=" << result.last_insert_id << "\n";
}

#if !defined(_WIN32)
bool send_all(int fd, const std::string& payload) {
    std::size_t sent = 0;
    while (sent < payload.size()) {
        const ssize_t n = ::send(fd, payload.data() + sent, payload.size() - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }
    return true;
}

bool recv_line(int fd, std::string& out_line) {
    out_line.clear();
    char ch = '\0';
    while (true) {
        const ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n == 0) {
            return false;
        }
        if (n < 0) {
            return false;
        }

        if (ch == '\n') {
            break;
        }

        if (ch != '\r') {
            out_line.push_back(ch);
        }
    }
    return true;
}

bool read_server_payload(int fd, std::string& header, std::string& payload) {
    header.clear();
    payload.clear();

    std::string line;
    if (!recv_line(fd, header)) {
        return false;
    }

    while (recv_line(fd, line)) {
        if (line == k_end_marker) {
            return true;
        }
        payload += line;
        payload.push_back('\n');
    }

    return false;
}

bool recv_datagram(int fd, std::string& payload) {
    char buffer[8192];
    const ssize_t n = ::recv(fd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        return false;
    }
    buffer[n] = '\0';
    payload.assign(buffer);
    return true;
}

bool parse_wire_payload(const std::string& wire, std::string& header, std::string& payload) {
    header.clear();
    payload.clear();

    const std::size_t first_nl = wire.find('\n');
    if (first_nl == std::string::npos) {
        return false;
    }

    header = wire.substr(0, first_nl);
    std::string rest = wire.substr(first_nl + 1);
    const std::string marker = std::string("\n") + k_end_marker + "\n";
    const std::size_t marker_pos = rest.find(marker);
    if (marker_pos == std::string::npos) {
        payload = rest;
        return true;
    }

    payload = rest.substr(0, marker_pos);
    return true;
}
#endif

}  // namespace

int main(int argc, const char* const argv[]) {
    using namespace os_generics::cli;

    CommandLineParser parser;
    parser.set_program_name("database_client");
    parser.set_description("Interactive SQL client for local database files.");
    parser.add_option({"help", 'h', false, false, "", "Show help and exit."});
    parser.add_option({"provider", 'p', true, false, "PROVIDER", "Database provider (default: sqlite)."});
    parser.add_option({"db", 'd', true, false, "PATH", "Database path (default: database_driver.sqlite3)."});
    parser.add_option({"host", '\0', true, false, "HOST", "Remote database server host (enables TCP mode)."});
    parser.add_option({"port", '\0', true, false, "PORT", "Remote database server TCP port (default: 55432)."});
    parser.add_option({"transport", '\0', true, false, "tcp|udp", "Remote transport in host mode (default: tcp)."});

    const auto parsed = parser.parse_argv(argc, argv);
    if (!parsed.ok()) {
        for (const auto& err : parsed.errors) {
            std::cerr << "Error: " << err << "\n";
        }
        std::cerr << "\n" << parser.render_help() << "\n";
        return 1;
    }

    if (parsed.has_option("help")) {
        std::cout << parser.render_help() << "\n";
        return 0;
    }

    const std::string provider = parsed.option_value("provider", "sqlite");
    const std::string db_path = parsed.option_value("db", "database_driver.sqlite3");
    const std::string host = parsed.option_value("host", "");
    const int port = std::stoi(parsed.option_value("port", std::to_string(k_default_port)));
    const std::string transport = parsed.option_value("transport", "tcp");

    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port: " << port << "\n";
        return 1;
    }

    const bool remote_mode = !host.empty();

#if !defined(_WIN32)
    if (remote_mode) {
        if (transport == "udp") {
            const int fd = ::socket(AF_INET, SOCK_DGRAM, 0);
            if (fd < 0) {
                std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
                return 1;
            }

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(static_cast<uint16_t>(port));
            if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
                std::cerr << "Invalid host: " << host << "\n";
                ::close(fd);
                return 1;
            }

            if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
                std::cerr << "connect() failed: " << std::strerror(errno) << "\n";
                ::close(fd);
                return 1;
            }

            std::cout << "database_client connected via udp://" << host << ':' << port << "\n";
            std::cout << "Type SQL and press Enter. Use .exit to quit.\n";

            std::string line;
            while (true) {
                std::cout << "db> ";
                if (!std::getline(std::cin, line)) {
                    break;
                }
                if (line.empty()) {
                    continue;
                }

                if (!send_all(fd, line)) {
                    // send_all uses send(2), and works with connected UDP sockets.
                    std::cerr << "Failed to send UDP request\n";
                    break;
                }

                std::string wire;
                if (!recv_datagram(fd, wire)) {
                    std::cerr << "No UDP response from server\n";
                    break;
                }

                std::string header;
                std::string payload;
                if (!parse_wire_payload(wire, header, payload)) {
                    std::cerr << "Malformed UDP response\n";
                    break;
                }

                if (header == "ERR") {
                    std::cerr << payload << "\n";
                } else {
                    std::cout << payload << "\n";
                }

                if (line == ".exit" || line == "quit" || line == "exit") {
                    break;
                }
            }

            ::close(fd);
            return 0;
        }

        const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            std::cerr << "socket() failed: " << std::strerror(errno) << "\n";
            return 1;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
            std::cerr << "Invalid host: " << host << "\n";
            ::close(fd);
            return 1;
        }

        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "connect() failed: " << std::strerror(errno) << "\n";
            ::close(fd);
            return 1;
        }

        std::string banner_header;
        std::string banner_payload;
        if (!read_server_payload(fd, banner_header, banner_payload)) {
            std::cerr << "Failed to read server banner\n";
            ::close(fd);
            return 1;
        }

        std::cout << "database_client connected to tcp://" << host << ':' << port << "\n";
        std::cout << "Type SQL and press Enter. Use .exit to quit.\n";

        std::string line;
        while (true) {
            std::cout << "db> ";
            if (!std::getline(std::cin, line)) {
                break;
            }

            if (line.empty()) {
                continue;
            }

            if (!send_all(fd, line + "\n")) {
                std::cerr << "Failed to send request to server\n";
                break;
            }

            std::string header;
            std::string payload;
            if (!read_server_payload(fd, header, payload)) {
                std::cerr << "Connection closed by server\n";
                break;
            }

            if (header == "ERR") {
                std::cerr << payload;
            } else {
                std::cout << payload;
            }

            if (line == ".exit" || line == "quit" || line == "exit") {
                break;
            }
        }

        ::close(fd);
        return 0;
    }
#else
    if (remote_mode) {
        std::cerr << "Remote TCP mode is currently supported on macOS/Linux builds.\n";
        return 1;
    }
#endif

    try {
        std::unique_ptr<ml::sql::Database> db = ml::sql::Database::create(provider);
        if (!db->connect(db_path)) {
            std::cerr << "Failed to connect to database: " << db_path << "\n";
            return 1;
        }

        std::cout << "database_client connected to " << db_path << " (provider=" << provider << ")\n";
        std::cout << "Type SQL and press Enter. Use .exit to quit.\n";

        std::string line;
        while (true) {
            std::cout << "db> ";
            if (!std::getline(std::cin, line)) {
                break;
            }

            if (line == ".exit" || line == "quit" || line == "exit") {
                break;
            }

            if (line.empty()) {
                continue;
            }

            try {
                const auto result = db->execute(line);
                print_result(result);
            } catch (const std::exception& ex) {
                std::cerr << "SQL error: " << ex.what() << "\n";
            }
        }

        db->disconnect();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "database_client error: " << ex.what() << "\n";
        return 1;
    }
}
