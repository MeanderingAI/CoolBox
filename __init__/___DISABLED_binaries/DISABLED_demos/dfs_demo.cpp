#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include "services/distributed_fs/distributed_fs.h"

using namespace services::distributed_fs;

void print_separator() {
    std::cout << "─────────────────────────────────────────\n";
}

void print_file_info(const FileMetadata& metadata) {
    std::cout << "  File ID: " << metadata.file_id << "\n";
    std::cout << "  Path: " << metadata.path << "\n";
    std::cout << "  Size: " << metadata.total_size << " bytes\n";
    std::cout << "  Chunks: " << metadata.num_chunks << "\n";
    std::cout << "  Chunk Size: " << metadata.chunk_size << " bytes\n";
    std::cout << "  Replication: " << metadata.replication_factor << "x\n";
}

void demo_basic_operations() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Basic File Operations Demo          ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    DFSClient client("localhost", 9000);
    
    if (!client.connect()) {
        std::cerr << "Failed to connect to DFS\n";
        return;
    }
    
    std::cout << "✓ Connected to Distributed File System\n\n";
    
    // Write a file
    std::string content = "Hello, Distributed File System! This is a test file.";
    std::vector<char> data(content.begin(), content.end());
    
    std::cout << "Writing file /test/hello.txt...\n";
    if (client.write_data("/test/hello.txt", data)) {
        std::cout << "✓ File written successfully\n";
        std::cout << "  Size: " << data.size() << " bytes\n";
    }
    
    // Read the file back
    std::cout << "\nReading file /test/hello.txt...\n";
    std::vector<char> read_data;
    if (client.read_data("/test/hello.txt", read_data)) {
        std::cout << "✓ File read successfully\n";
        std::cout << "  Size: " << read_data.size() << " bytes\n";
        std::cout << "  Content: " << std::string(read_data.begin(), read_data.end()) << "\n";
    }
    
    // Check if file exists
    std::cout << "\nChecking if file exists...\n";
    if (client.file_exists("/test/hello.txt")) {
        std::cout << "✓ File exists\n";
    }
    
    // Get file info
    std::cout << "\nGetting file information...\n";
    auto metadata = client.get_file_info("/test/hello.txt");
    print_file_info(metadata);
}

void demo_directory_operations() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Directory Operations Demo           ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    DFSClient client("localhost", 9000);
    client.connect();
    
    // Create multiple files
    std::cout << "Creating multiple files...\n";
    
    std::vector<std::string> files = {
        "/docs/readme.txt",
        "/docs/guide.txt",
        "/data/records.dat",
        "/config/settings.conf"
    };
    
    for (const auto& path : files) {
        std::string content = "Content of " + path;
        std::vector<char> data(content.begin(), content.end());
        
        if (client.write_data(path, data)) {
            std::cout << "  ✓ Created: " << path << "\n";
        }
    }
    
    // List directory
    std::cout << "\nListing /docs directory:\n";
    print_separator();
    auto docs_files = client.list_directory("/docs");
    for (const auto& file : docs_files) {
        std::cout << "  📄 " << file << "\n";
    }
    
    std::cout << "\nListing all files:\n";
    print_separator();
    auto all_files = client.list_directory("/");
    for (const auto& file : all_files) {
        std::cout << "  📄 " << file << "\n";
    }
    
    std::cout << "\nTotal files: " << all_files.size() << "\n";
}

void demo_file_operations() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Advanced File Operations Demo       ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    DFSClient client("localhost", 9000);
    client.connect();
    
    // Create source file
    std::string content = "Original content for testing file operations.";
    std::vector<char> data(content.begin(), content.end());
    
    std::cout << "Creating source file /source.txt...\n";
    client.write_data("/source.txt", data);
    std::cout << "✓ File created\n";
    
    // Copy file
    std::cout << "\nCopying /source.txt to /backup.txt...\n";
    if (client.copy_file("/source.txt", "/backup.txt")) {
        std::cout << "✓ File copied successfully\n";
    }
    
    // Verify both exist
    std::cout << "\nVerifying files:\n";
    std::cout << "  /source.txt exists: " << (client.file_exists("/source.txt") ? "Yes" : "No") << "\n";
    std::cout << "  /backup.txt exists: " << (client.file_exists("/backup.txt") ? "Yes" : "No") << "\n";
    
    // Move file
    std::cout << "\nMoving /backup.txt to /archive.txt...\n";
    if (client.move_file("/backup.txt", "/archive.txt")) {
        std::cout << "✓ File moved successfully\n";
    }
    
    std::cout << "\nVerifying after move:\n";
    std::cout << "  /backup.txt exists: " << (client.file_exists("/backup.txt") ? "Yes" : "No") << "\n";
    std::cout << "  /archive.txt exists: " << (client.file_exists("/archive.txt") ? "Yes" : "No") << "\n";
    
    // Delete file
    std::cout << "\nDeleting /source.txt...\n";
    if (client.delete_file("/source.txt")) {
        std::cout << "✓ File deleted successfully\n";
    }
    
    std::cout << "\nVerifying after delete:\n";
    std::cout << "  /source.txt exists: " << (client.file_exists("/source.txt") ? "Yes" : "No") << "\n";
}

void demo_large_files() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Large File Handling Demo            ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    DFSClient client("localhost", 9000);
    client.connect();
    
    // Create a large file (10 MB)
    size_t file_size = 10 * 1024 * 1024;  // 10 MB
    std::cout << "Creating large file (10 MB)...\n";
    
    std::vector<char> large_data(file_size);
    for (size_t i = 0; i < file_size; i++) {
        large_data[i] = static_cast<char>(i % 256);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    if (client.write_data("/large/bigfile.dat", large_data)) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Large file written successfully\n";
        std::cout << "  Size: " << file_size << " bytes (" 
                  << std::fixed << std::setprecision(2) 
                  << (file_size / (1024.0 * 1024.0)) << " MB)\n";
        std::cout << "  Time: " << duration.count() << " ms\n";
        std::cout << "  Speed: " << std::fixed << std::setprecision(2)
                  << (file_size / (1024.0 * 1024.0)) / (duration.count() / 1000.0) 
                  << " MB/s\n";
        
        // Get file metadata
        auto metadata = client.get_file_info("/large/bigfile.dat");
        std::cout << "\n  File Metadata:\n";
        std::cout << "    Total chunks: " << metadata.num_chunks << "\n";
        std::cout << "    Chunk size: " << metadata.chunk_size << " bytes\n";
        std::cout << "    Replication factor: " << metadata.replication_factor << "x\n";
    }
    
    // Read it back
    std::cout << "\nReading large file back...\n";
    start = std::chrono::high_resolution_clock::now();
    
    std::vector<char> read_data;
    if (client.read_data("/large/bigfile.dat", read_data)) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Large file read successfully\n";
        std::cout << "  Size: " << read_data.size() << " bytes\n";
        std::cout << "  Time: " << duration.count() << " ms\n";
        std::cout << "  Speed: " << std::fixed << std::setprecision(2)
                  << (read_data.size() / (1024.0 * 1024.0)) / (duration.count() / 1000.0) 
                  << " MB/s\n";
        
        // Verify data integrity
        bool data_matches = (large_data == read_data);
        std::cout << "  Data integrity: " << (data_matches ? "✓ Verified" : "✗ Failed") << "\n";
    }
}

void demo_statistics() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   System Statistics Demo              ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    DFSClient client("localhost", 9000);
    client.connect();
    
    // Create sample files
    std::cout << "Creating sample files for statistics...\n";
    
    for (int i = 1; i <= 5; i++) {
        std::string path = "/stats/file" + std::to_string(i) + ".txt";
        std::string content = "Sample file " + std::to_string(i) + " with some content.";
        std::vector<char> data(content.begin(), content.end());
        client.write_data(path, data);
    }
    
    std::cout << "✓ Sample files created\n\n";
    
    // Get all files
    auto all_files = client.list_directory("/");
    
    std::cout << "System Statistics:\n";
    print_separator();
    std::cout << "  Total Files: " << all_files.size() << "\n";
    
    size_t total_size = 0;
    size_t total_chunks = 0;
    
    for (const auto& path : all_files) {
        auto metadata = client.get_file_info(path);
        total_size += metadata.total_size;
        total_chunks += metadata.num_chunks;
    }
    
    std::cout << "  Total Size: " << total_size << " bytes ("
              << std::fixed << std::setprecision(2)
              << (total_size / 1024.0) << " KB)\n";
    std::cout << "  Total Chunks: " << total_chunks << "\n";
    std::cout << "  Average File Size: " 
              << (all_files.empty() ? 0 : total_size / all_files.size()) << " bytes\n";
    
    std::cout << "\nFile List:\n";
    print_separator();
    for (const auto& path : all_files) {
        auto metadata = client.get_file_info(path);
        std::cout << "  📄 " << std::setw(30) << std::left << path 
                  << "  " << std::setw(10) << std::right << metadata.total_size << " bytes\n";
    }
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║       Distributed File System Demo                ║\n";
    std::cout << "║       Chunk-based Storage & Replication           ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    demo_basic_operations();
    demo_directory_operations();
    demo_file_operations();
    demo_large_files();
    demo_statistics();
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Demo Complete!                      ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    std::cout << "Distributed File System Features:\n";
    std::cout << "  ✓ Chunk-based storage (4MB chunks)\n";
    std::cout << "  ✓ Configurable replication factor\n";
    std::cout << "  ✓ File metadata management\n";
    std::cout << "  ✓ Directory operations\n";
    std::cout << "  ✓ Large file support\n";
    std::cout << "  ✓ Copy, move, delete operations\n";
    std::cout << "  ✓ Data integrity verification\n\n";
    
    return 0;
}
