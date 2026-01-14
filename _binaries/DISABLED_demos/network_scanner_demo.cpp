#include "security/network_scanner/network_scanner.h"
#include <iostream>
#include <fstream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    std::cout << "╔══════════════════════════════════════════════════╗\n";
    std::cout << "║      Network Scanner & Mapper                    ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n\n";

    // Parse command line arguments
    std::string output_file;
    if (argc > 1) {
        output_file = argv[1];
        std::cout << "Topology will be exported to: " << output_file << "\n\n";
    }

    // Configure scanner
    security::ScanConfig config;
    config.ports_to_scan = {22, 80, 443, 8080, 8443, 9000, 9001, 9002};
    config.scan_tcp = true;
    config.scan_udp = false;
    config.scan_http = true;
    config.perform_fuzzing = true;  // Enable fuzzing to test for vulnerabilities
    config.timeout_ms = 500;
    config.verbose = true;

    security::NetworkScanner scanner(config);

    // Scan localhost and common private network
    std::cout << "=== Scanning Localhost ===\n";
    scanner.scan_single_host("127.0.0.1");

    // You can scan a network range like this:
    // std::cout << "\n=== Scanning Local Network ===\n";
    // scanner.scan_network("192.168.1.1-10");
    // Or using CIDR notation:
    // scanner.scan_network("192.168.1.0/24");

    // Print results
    scanner.print_results();
    scanner.print_topology_ascii();

    // Get statistics
    auto stats = scanner.get_statistics();
    std::cout << "\n╔═══════════════════════════════════════════════════╗\n";
    std::cout << "║         Scan Statistics                           ║\n";
    std::cout << "╚═══════════════════════════════════════════════════╝\n";
    std::cout << "Total Hosts Scanned:    " << stats["total_hosts"] << "\n";
    std::cout << "Alive Hosts:            " << stats["alive_hosts"] << "\n";
    std::cout << "Open Ports Found:       " << stats["open_ports"] << "\n";
    std::cout << "Vulnerable Hosts:       " << stats["vulnerable_hosts"] << "\n\n";

    // Export topology if output file specified
    if (!output_file.empty()) {
        try {
            scanner.export_topology(output_file, "dot");
            
            // Try to generate PNG if graphviz is installed
            std::string png_file = output_file.substr(0, output_file.rfind('.')) + ".png";
            std::string cmd = "dot -Tpng " + output_file + " -o " + png_file + " 2>/dev/null";
            if (system(cmd.c_str()) == 0) {
                std::cout << "Topology image created: " << png_file << "\n";
            }
        } catch (const std::exception& e) {
            std::cerr << "Error exporting topology: " << e.what() << "\n";
        }
    } else {
        std::cout << "Topology Graph (DOT format):\n";
        std::cout << "────────────────────────────────────────────\n";
        
        // Print DOT graph to console
        security::NetworkScanner temp_scanner(config);
        temp_scanner.scan_single_host("127.0.0.1");
        std::ofstream temp_file("/tmp/network_topology.dot");
        temp_scanner.export_topology("/tmp/network_topology.dot", "dot");
        
        std::ifstream dot_file("/tmp/network_topology.dot");
        std::string line;
        while (std::getline(dot_file, line)) {
            std::cout << line << "\n";
        }
        dot_file.close();
        
        std::cout << "────────────────────────────────────────────\n";
        std::cout << "\nTo generate a visual graph, run with output file:\n";
        std::cout << "  " << argv[0] << " network_topology.dot\n";
        std::cout << "Then create image with:\n";
        std::cout << "  dot -Tpng network_topology.dot -o network_topology.png\n";
    }

    std::cout << "\n╔══════════════════════════════════════════════════╗\n";
    std::cout << "║         Scan Complete!                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════╝\n";

    return 0;
}
