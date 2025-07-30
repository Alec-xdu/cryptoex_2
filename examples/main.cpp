#include "psi_protocol.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>

using namespace SecretSharingPSI;

void printSeparator(const std::string& title) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "  " << title << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

void printStatistics(const PSIProtocol::Statistics& stats, const std::string& party_name) {
    std::cout << "\n" << party_name << " Statistics:" << std::endl;
    std::cout << "  Input Set Size: " << stats.input_set_size << std::endl;
    std::cout << "  OPRF Processed: " << stats.oprf_processed_count << std::endl;
    std::cout << "  Hash Table Size: " << stats.hash_table_size << std::endl;
    std::cout << "  Load Factor: " << std::fixed << std::setprecision(3) 
              << stats.hash_table_load_factor << std::endl;
    std::cout << "  Intersection Size: " << stats.intersection_size << std::endl;
}

void printPSIResult(const PSIResult& result) {
    std::cout << "\nPSI Protocol Results:" << std::endl;
    std::cout << "  Sender Set Size: " << result.sender_set_size << std::endl;
    std::cout << "  Receiver Set Size: " << result.receiver_set_size << std::endl;
    std::cout << "  Intersection Size: " << result.intersection_size << std::endl;
    
    if (!result.intersection.empty()) {
        std::cout << "  Intersection Items:" << std::endl;
        for (size_t i = 0; i < std::min(result.intersection.size(), size_t(10)); ++i) {
            std::cout << "    " << (i + 1) << ". " << result.intersection[i].toString() << std::endl;
        }
        if (result.intersection.size() > 10) {
            std::cout << "    ... and " << (result.intersection.size() - 10) << " more items" << std::endl;
        }
    }
}

std::vector<Item> generateTestSet(const std::string& prefix, size_t count, size_t start_id = 0) {
    std::vector<Item> items;
    items.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        std::string data = prefix + std::to_string(start_id + i);
        items.emplace_back(data);
    }
    
    return items;
}

void demonstrateSecretSharing() {
    printSeparator("Secret Sharing Demonstration");
    
    try {
        // 创建TEE实例
        SecretSharing tee;
        
        // 生成种子
        Seed seed_a, seed_b;
        if (!tee.generateSeeds(seed_a, seed_b)) {
            std::cerr << "Failed to generate seeds" << std::endl;
            return;
        }
        std::cout << "✓ TEE generated seeds for both parties" << std::endl;
        
        // TEE为参与方B生成预处理数据
        auto party_b_data = tee.generateDataForB(seed_a, seed_b, 10);
        std::cout << "✓ TEE generated " << party_b_data.pairs.size() 
                  << " beaver triples for Party B" << std::endl;
        
        // 初始化参与方A
        SecretSharing party_a;
        party_a.initializePartyA(seed_a, 10);
        std::cout << "✓ Party A initialized with seed" << std::endl;
        
        // 初始化参与方B
        SecretSharing party_b;
        party_b.initializePartyB(seed_b, party_b_data.pairs, party_b_data.c1_values);
        std::cout << "✓ Party B initialized with seed and pre-processed data" << std::endl;
        
        // 演示安全乘法
        uint64_t x = 123, y = 456;
        uint64_t x_share_a = 50, x_share_b = x - x_share_a;
        uint64_t y_share_a = 100, y_share_b = y - y_share_a;
        
        std::cout << "\nTesting secure multiplication:" << std::endl;
        std::cout << "  x = " << x << " (shared as " << x_share_a << " + " << x_share_b << ")" << std::endl;
        std::cout << "  y = " << y << " (shared as " << y_share_a << " + " << y_share_b << ")" << std::endl;
        std::cout << "  Expected result: " << (x * y) << std::endl;
        
        // 注意：实际的安全乘法需要通信，这里只是演示API
        std::cout << "✓ Secret sharing setup completed successfully" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Secret sharing demonstration failed: " << e.what() << std::endl;
    }
}

void demonstratePSIProtocol() {
    printSeparator("PSI Protocol Demonstration");
    
    try {
        // 创建测试数据集
        auto sender_set = generateTestSet("item_", 100, 1);     // item_1 到 item_100
        auto receiver_set = generateTestSet("item_", 100, 50);  // item_50 到 item_149
        // 预期交集：item_50 到 item_100 (51个元素)
        
        std::cout << "Generated test sets:" << std::endl;
        std::cout << "  Sender set: " << sender_set.size() << " items (item_1 to item_100)" << std::endl;
        std::cout << "  Receiver set: " << receiver_set.size() << " items (item_50 to item_149)" << std::endl;
        std::cout << "  Expected intersection: 51 items (item_50 to item_100)" << std::endl;
        
        // 创建协议实例
        PSIProtocol::Config config;
        config.hash_table_capacity = 512;
        config.beaver_triple_count = 200;
        
        PSIProtocol sender(PSIProtocol::Role::SENDER, config);
        PSIProtocol receiver(PSIProtocol::Role::RECEIVER, config);
        PSIProtocol tee(PSIProtocol::Role::TEE, config);
        
        // 初始化协议
        if (!sender.initialize() || !receiver.initialize() || !tee.initialize()) {
            std::cerr << "Failed to initialize protocols" << std::endl;
            return;
        }
        std::cout << "✓ All protocols initialized" << std::endl;
        
        // TEE生成种子和预处理数据
        Seed seed_a, seed_b;
        if (!tee.generateSeeds(seed_a, seed_b)) {
            std::cerr << "Failed to generate seeds" << std::endl;
            return;
        }
        
        auto party_b_data = tee.generatePartyBData(seed_a, seed_b);
        std::cout << "✓ TEE generated seeds and preprocessing data" << std::endl;
        
        // 初始化秘密分享
        if (!sender.initializeSecretSharing(seed_a) || 
            !receiver.initializeSecretSharing(seed_b, &party_b_data)) {
            std::cerr << "Failed to initialize secret sharing" << std::endl;
            return;
        }
        std::cout << "✓ Secret sharing initialized for both parties" << std::endl;
        
        // 设置输入数据集
        if (!sender.setInputSet(sender_set) || !receiver.setInputSet(receiver_set)) {
            std::cerr << "Failed to set input sets" << std::endl;
            return;
        }
        std::cout << "✓ Input sets configured" << std::endl;
        
        // 执行PSI协议
        auto start_time = std::chrono::high_resolution_clock::now();
        
        PSIResult result = sender.runProtocol(receiver);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "✓ PSI protocol completed in " << duration.count() << " ms" << std::endl;
        
        // 显示结果
        printPSIResult(result);
        
        // 显示统计信息
        printStatistics(sender.getStatistics(), "Sender");
        printStatistics(receiver.getStatistics(), "Receiver");
        
        // 验证结果正确性
        if (result.intersection_size == 51) {
            std::cout << "\n✓ PSI result verification: PASSED" << std::endl;
        } else {
            std::cout << "\n✗ PSI result verification: FAILED" << std::endl;
            std::cout << "  Expected 51 items, got " << result.intersection_size << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "PSI protocol demonstration failed: " << e.what() << std::endl;
    }
}

void demonstrateHashTables() {
    printSeparator("Hash Table Performance Test");
    
    try {
        const size_t test_size = 1000;
        
        // 测试普通哈希表
        HashTable regular_table(2048, 0.75);
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < test_size; ++i) {
            regular_table.insert(i, i * 2);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "Regular Hash Table:" << std::endl;
        std::cout << "  Inserted " << test_size << " items in " << duration.count() << " μs" << std::endl;
        
        auto stats = regular_table.getStatistics();
        std::cout << "  Load Factor: " << std::fixed << std::setprecision(3) << stats.load_factor << std::endl;
        std::cout << "  Collisions: " << stats.collision_count << std::endl;
        std::cout << "  Avg Probe Distance: " << std::fixed << std::setprecision(2) 
                  << stats.average_probe_distance << std::endl;
        
        // 测试布谷鸟哈希表
        CuckooHashTable cuckoo_table(2048, 8);
        start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < test_size; ++i) {
            cuckoo_table.insert(i, i * 2);
        }
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        std::cout << "\nCuckoo Hash Table:" << std::endl;
        std::cout << "  Inserted " << test_size << " items in " << duration.count() << " μs" << std::endl;
        
        auto cuckoo_stats = cuckoo_table.getStatistics();
        std::cout << "  Load Factor: " << std::fixed << std::setprecision(3) << cuckoo_stats.load_factor << std::endl;
        std::cout << "  Rehash Count: " << cuckoo_stats.rehash_count << std::endl;
        std::cout << "  Kick Count: " << cuckoo_stats.kick_count << std::endl;
        std::cout << "  Success Rate: " << std::fixed << std::setprecision(3) << cuckoo_stats.success_rate << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Hash table demonstration failed: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Secret Sharing PSI Protocol Demonstration" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    try {
        // 演示秘密分享
        demonstrateSecretSharing();
        
        // 演示哈希表性能
        demonstrateHashTables();
        
        // 演示完整PSI协议
        demonstratePSIProtocol();
        
        printSeparator("Demonstration Complete");
        std::cout << "All demonstrations completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Demonstration failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}