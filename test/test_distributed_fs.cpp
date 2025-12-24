#include <gtest/gtest.h>
#include "services/distributed_fs/distributed_fs.h"
#include <thread>
#include <chrono>
#include <fstream>
#include <filesystem>

using namespace services::distributed_fs;

class DistributedFSTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a test directory
        test_dir = "./test_dfs_" + std::to_string(std::time(nullptr));
        std::filesystem::create_directories(test_dir);
        
        // Initialize DFS master node
        master = std::make_unique<MasterNode>(9000);
        master->set_chunk_size(1024 * 1024); // 1MB chunks
        master->set_replication_factor(3);
        master->start();
        
        // Register some virtual storage nodes (without actually creating them)
        for (int i = 0; i < 3; ++i) {
            StorageNodeInfo info;
            info.node_id = "node" + std::to_string(i);
            info.address = "localhost";
            info.port = 9000 + i;
            info.capacity = 100 * 1024 * 1024;
            info.available_space = 100 * 1024 * 1024;
            info.used_space = 0;
            info.is_alive = true;
            info.last_heartbeat = std::chrono::system_clock::now();
            master->register_node(info);
        }
        
        // Create client
        client = std::make_unique<DFSClient>("localhost", 9000);
        client->connect();
    }
    
    void TearDown() override {
        // Disconnect client
        if (client) {
            client->disconnect();
        }
        
        // Stop master
        if (master) {
            master->stop();
        }
        
        // Clean up test directory
        std::filesystem::remove_all(test_dir);
    }
    
    std::string test_dir;
    std::unique_ptr<MasterNode> master;
    std::unique_ptr<DFSClient> client;
};

// Test basic file creation
TEST_F(DistributedFSTest, CreateFile) {
    std::string path = "/test/file.txt";
    std::vector<char> data = {'H', 'e', 'l', 'l', 'o'};
    
    auto result = master->create_file(path, data);
    EXPECT_TRUE(result.success);
    
    auto metadata = master->get_file_metadata(path);
    EXPECT_NE(metadata, nullptr);
}

// Test file doesn't exist initially
TEST_F(DistributedFSTest, FileNotExists) {
    EXPECT_FALSE(client->file_exists("/nonexistent/file.txt"));
}

// Test reading a file
TEST_F(DistributedFSTest, ReadFile) {
    std::string path = "/test/data.bin";
    std::vector<char> original_data = {'A', 'B', 'C', 'D', 'E'};
    
    auto create_result = master->create_file(path, original_data);
    EXPECT_TRUE(create_result.success);
    
    std::vector<char> read_data;
    auto read_result = master->read_file(path, read_data);
    EXPECT_TRUE(read_result.success);
    EXPECT_EQ(original_data, read_data);
}

// Test updating a file
TEST_F(DistributedFSTest, UpdateFile) {
    std::string path = "/test/update.txt";
    std::vector<char> data1 = {'O', 'l', 'd'};
    std::vector<char> data2 = {'N', 'e', 'w'};
    
    EXPECT_TRUE(master->create_file(path, data1).success);
    EXPECT_TRUE(master->update_file(path, data2).success);
    
    std::vector<char> read_data;
    EXPECT_TRUE(master->read_file(path, read_data).success);
    EXPECT_EQ(data2, read_data);
}

// Test deleting a file
TEST_F(DistributedFSTest, DeleteFile) {
    std::string path = "/test/delete.txt";
    std::vector<char> data = {'D', 'e', 'l', 'e', 't', 'e'};
    
    EXPECT_TRUE(master->create_file(path, data).success);
    EXPECT_NE(master->get_file_metadata(path), nullptr);
    
    EXPECT_TRUE(master->delete_file(path).success);
    EXPECT_EQ(master->get_file_metadata(path), nullptr);
}

// Test listing files in a directory
TEST_F(DistributedFSTest, ListFiles) {
    master->create_file("/dir/file1.txt", std::vector<char>{'1'});
    master->create_file("/dir/file2.txt", std::vector<char>{'2'});
    master->create_file("/dir/subdir/file3.txt", std::vector<char>{'3'});
    master->create_file("/other/file4.txt", std::vector<char>{'4'});
    
    auto files = master->list_files("/dir");
    
    EXPECT_GE(files.size(), 2);
    bool has_file1 = false, has_file2 = false;
    for (const auto& f : files) {
        if (f.find("file1.txt") != std::string::npos) has_file1 = true;
        if (f.find("file2.txt") != std::string::npos) has_file2 = true;
    }
    EXPECT_TRUE(has_file1);
    EXPECT_TRUE(has_file2);
}

// Test getting file metadata
TEST_F(DistributedFSTest, GetFileInfo) {
    std::string path = "/test/info.txt";
    std::vector<char> data(1024, 'X');
    
    EXPECT_TRUE(master->create_file(path, data).success);
    
    auto info = master->get_file_metadata(path);
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(info->path, path);
    EXPECT_EQ(info->total_size, 1024);
    EXPECT_GT(info->chunks.size(), 0);
}

// Test file info for non-existent file
TEST_F(DistributedFSTest, GetFileInfoNotExists) {
    auto info = master->get_file_metadata("/nonexistent.txt");
    EXPECT_EQ(info, nullptr);
}

// Test large file handling (multiple chunks)
TEST_F(DistributedFSTest, LargeFile) {
    std::string path = "/test/large.bin";
    // Create data larger than chunk size (1MB in this test setup)
    std::vector<char> data(2 * 1024 * 1024, 'L'); // 2 MB
    
    EXPECT_TRUE(master->create_file(path, data).success);
    
    auto info = master->get_file_metadata(path);
    EXPECT_NE(info, nullptr);
    EXPECT_EQ(info->total_size, data.size());
    EXPECT_GE(info->chunks.size(), 2); // Should be split into at least 2 chunks
    
    std::vector<char> read_data;
    EXPECT_TRUE(master->read_file(path, read_data).success);
    EXPECT_EQ(data, read_data);
}

// Test empty file
TEST_F(DistributedFSTest, EmptyFile) {
    std::string path = "/test/empty.txt";
    std::vector<char> data;
    
    EXPECT_TRUE(master->create_file(path, data).success);
    EXPECT_NE(master->get_file_metadata(path), nullptr);
    
    std::vector<char> read_data;
    EXPECT_TRUE(master->read_file(path, read_data).success);
    EXPECT_TRUE(read_data.empty());
}

// Test reading non-existent file
TEST_F(DistributedFSTest, ReadNonExistentFile) {
    std::vector<char> data;
    EXPECT_FALSE(master->read_file("/nonexistent.txt", data).success);
}

// Test updating non-existent file
TEST_F(DistributedFSTest, UpdateNonExistentFile) {
    std::vector<char> data = {'X'};
    EXPECT_FALSE(master->update_file("/nonexistent.txt", data).success);
}

// Test deleting non-existent file
TEST_F(DistributedFSTest, DeleteNonExistentFile) {
    EXPECT_FALSE(master->delete_file("/nonexistent.txt").success);
}

// Test storage node registration
TEST_F(DistributedFSTest, StorageNodeRegistration) {
    auto nodes = master->get_active_nodes();
    EXPECT_EQ(nodes.size(), 3);
    
    for (const auto& node : nodes) {
        EXPECT_TRUE(node.is_alive);
        EXPECT_GT(node.available_space, 0);
    }
}

// Test path normalization
TEST_F(DistributedFSTest, PathNormalization) {
    // Create files with different path formats
    auto result1 = master->create_file("/test//double//slash.txt", std::vector<char>{'A'});
    auto result2 = master->create_file("/test/./dot/file.txt", std::vector<char>{'B'});
    
    // Files should be created successfully (path normalization happens internally)
    EXPECT_TRUE(result1.success);
    EXPECT_TRUE(result2.success);
}

// Test binary data integrity
TEST_F(DistributedFSTest, BinaryDataIntegrity) {
    std::string path = "/test/binary.dat";
    
    // Create binary data with all byte values
    std::vector<char> data(256);
    for (int i = 0; i < 256; ++i) {
        data[i] = static_cast<char>(i);
    }
    
    EXPECT_TRUE(master->create_file(path, data).success);
    
    std::vector<char> read_data;
    EXPECT_TRUE(master->read_file(path, read_data).success);
    EXPECT_EQ(data, read_data);
    
    // Verify every byte
    for (size_t i = 0; i < data.size(); ++i) {
        EXPECT_EQ(data[i], read_data[i]) << "Mismatch at byte " << i;
    }
}

// Test replication factor
TEST_F(DistributedFSTest, ReplicationFactor) {
    std::string path = "/test/replicated.txt";
    std::vector<char> data(512, 'R');
    
    // Set replication factor
    master->set_replication_factor(2);
    EXPECT_EQ(master->get_replication_factor(), 2);
    
    // Create file with replication
    EXPECT_TRUE(master->create_file(path, data).success);
    
    auto info = master->get_file_metadata(path);
    EXPECT_NE(info, nullptr);
    
    // File should still be readable
    std::vector<char> read_data;
    EXPECT_TRUE(master->read_file(path, read_data).success);
    EXPECT_EQ(data, read_data);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
