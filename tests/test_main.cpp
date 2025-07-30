#include "psi_protocol.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <string>
#include <functional>

using namespace SecretSharingPSI;

// 简单的测试框架
class TestRunner {
public:
    static void run_test(const std::string& test_name, std::function<bool()> test_func) {
        std::cout << "Running " << test_name << "... ";
        try {
            if (test_func()) {
                std::cout << "PASSED" << std::endl;
                passed_++;
            } else {
                std::cout << "FAILED" << std::endl;
                failed_++;
            }
        } catch (const std::exception& e) {
            std::cout << "FAILED (Exception: " << e.what() << ")" << std::endl;
            failed_++;
        }
        total_++;
    }
    
    static void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total: " << total_ << std::endl;
        std::cout << "Passed: " << passed_ << std::endl;
        std::cout << "Failed: " << failed_ << std::endl;
        std::cout << "Success Rate: " << (total_ > 0 ? (100.0 * passed_ / total_) : 0) << "%" << std::endl;
    }
    
    static bool all_passed() { return failed_ == 0; }

private:
    static int total_, passed_, failed_;
};

int TestRunner::total_ = 0;
int TestRunner::passed_ = 0;
int TestRunner::failed_ = 0;

// Item类测试
bool test_item_basic() {
    Item item1("hello");
    Item item2("world");
    Item item3("hello");
    
    return item1 != item2 && item1 == item3 && 
           !item1.empty() && item1.size() == 5;
}

bool test_item_hash() {
    Item item1("test_data");
    Item item2("test_data");
    Item item3("different_data");
    
    Hash hash1 = item1.getHash();
    Hash hash2 = item2.getHash();
    Hash hash3 = item3.getHash();
    
    return hash1 == hash2 && hash1 != hash3;
}

bool test_item_value() {
    Item item1(12345ULL);
    uint64_t value = item1.getValue();
    
    return value == 12345ULL;
}

// SecretSharing类测试
bool test_secret_sharing_seeds() {
    SecretSharing ss;
    Seed seed_a, seed_b;
    
    bool result = ss.generateSeeds(seed_a, seed_b);
    return result && seed_a != seed_b;
}

bool test_secret_sharing_triples() {
    SecretSharing ss;
    Seed seed_a, seed_b;
    
    if (!ss.generateSeeds(seed_a, seed_b)) {
        return false;
    }
    
    auto triples_a = ss.generateTriplesForA(seed_a, 5);
    auto data_b = ss.generateDataForB(seed_a, seed_b, 5);
    
    return triples_a.size() == 5 && 
           data_b.pairs.size() == 5 && 
           data_b.c1_values.size() == 5;
}

bool test_secret_sharing_initialization() {
    SecretSharing ss;
    Seed seed_a, seed_b;
    
    if (!ss.generateSeeds(seed_a, seed_b)) {
        return false;
    }
    
    auto data_b = ss.generateDataForB(seed_a, seed_b, 10);
    
    SecretSharing party_a, party_b;
    party_a.initializePartyA(seed_a, 10);
    party_b.initializePartyB(seed_b, data_b.pairs, data_b.c1_values);
    
    // 测试获取三元组
    auto triple_a = party_a.getTripleA(0);
    auto data_b_0 = party_b.getDataB(0);
    
    return triple_a.a != 0 && data_b_0.pair.a != 0;
}

// OPRF类测试
bool test_oprf_basic() {
    OPRF oprf;
    
    if (!oprf.generateKey()) {
        return false;
    }
    
    return oprf.isInitialized();
}

bool test_oprf_process_item() {
    OPRF oprf;
    if (!oprf.generateKey()) {
        return false;
    }
    
    Item item("test_item");
    OPRFItem oprf_item = oprf.processItem(item);
    
    return oprf_item.original_item == item && 
           oprf_item.hash_value != 0;
}

bool test_oprf_batch_process() {
    OPRF oprf;
    if (!oprf.generateKey()) {
        return false;
    }
    
    std::vector<Item> items;
    for (int i = 0; i < 5; ++i) {
        items.emplace_back("item_" + std::to_string(i));
    }
    
    auto oprf_items = oprf.batchProcess(items);
    
    return oprf_items.size() == 5;
}

// HashTable类测试
bool test_hash_table_basic() {
    HashTable table(128);
    
    bool inserted = table.insert(123, 456);
    uint64_t value;
    bool found = table.find(123, value);
    
    return inserted && found && value == 456 && table.size() == 1;
}

bool test_hash_table_batch() {
    HashTable table(256);
    
    std::vector<OPRFItem> items;
    for (int i = 0; i < 10; ++i) {
        Item item("item_" + std::to_string(i));
        Hash dummy_hash;
        items.emplace_back(item, dummy_hash, i);
    }
    
    size_t inserted = table.batchInsert(items);
    
    return inserted == 10 && table.size() == 10;
}

// CuckooHashTable类测试
bool test_cuckoo_table_basic() {
    CuckooHashTable table(128);
    
    bool inserted = table.insert(123, 456);
    uint64_t value;
    bool found = table.find(123, value);
    
    return inserted && found && value == 456 && table.size() == 1;
}

bool test_cuckoo_table_batch() {
    CuckooHashTable table(256);
    
    std::vector<OPRFItem> items;
    for (int i = 0; i < 10; ++i) {
        Item item("item_" + std::to_string(i));
        Hash dummy_hash;
        items.emplace_back(item, dummy_hash, i);
    }
    
    size_t inserted = table.batchInsert(items);
    
    return inserted == 10 && table.size() == 10;
}

// PSIProtocol类测试
bool test_psi_protocol_initialization() {
    PSIProtocol sender(PSIProtocol::Role::SENDER);
    PSIProtocol receiver(PSIProtocol::Role::RECEIVER);
    PSIProtocol tee(PSIProtocol::Role::TEE);
    
    return sender.initialize() && receiver.initialize() && tee.initialize();
}

bool test_psi_protocol_seed_generation() {
    PSIProtocol tee(PSIProtocol::Role::TEE);
    if (!tee.initialize()) {
        return false;
    }
    
    Seed seed_a, seed_b;
    bool result = tee.generateSeeds(seed_a, seed_b);
    
    return result && seed_a != seed_b;
}

bool test_psi_protocol_input_set() {
    PSIProtocol sender(PSIProtocol::Role::SENDER);
    if (!sender.initialize()) {
        return false;
    }
    
    std::vector<Item> items;
    for (int i = 0; i < 5; ++i) {
        items.emplace_back("item_" + std::to_string(i));
    }
    
    bool result = sender.setInputSet(items);
    auto stats = sender.getStatistics();
    
    return result && stats.input_set_size == 5;
}

bool test_psi_protocol_complete() {
    // 创建测试数据
    std::vector<Item> sender_items, receiver_items;
    
    // 发送方：item_0 到 item_9
    for (int i = 0; i < 10; ++i) {
        sender_items.emplace_back("item_" + std::to_string(i));
    }
    
    // 接收方：item_5 到 item_14
    for (int i = 5; i < 15; ++i) {
        receiver_items.emplace_back("item_" + std::to_string(i));
    }
    
    // 预期交集：item_5 到 item_9 (5个元素)
    
    PSIProtocol::Config config;
    config.hash_table_capacity = 64;
    config.beaver_triple_count = 20;
    
    PSIProtocol sender(PSIProtocol::Role::SENDER, config);
    PSIProtocol receiver(PSIProtocol::Role::RECEIVER, config);
    PSIProtocol tee(PSIProtocol::Role::TEE, config);
    
    // 初始化
    if (!sender.initialize() || !receiver.initialize() || !tee.initialize()) {
        return false;
    }
    
    // TEE生成种子
    Seed seed_a, seed_b;
    if (!tee.generateSeeds(seed_a, seed_b)) {
        return false;
    }
    
    auto party_b_data = tee.generatePartyBData(seed_a, seed_b);
    
    // 初始化秘密分享
    if (!sender.initializeSecretSharing(seed_a) || 
        !receiver.initializeSecretSharing(seed_b, &party_b_data)) {
        return false;
    }
    
    // 设置输入
    if (!sender.setInputSet(sender_items) || !receiver.setInputSet(receiver_items)) {
        return false;
    }
    
    // 执行协议
    PSIResult result = sender.runProtocol(receiver);
    
    // 验证结果
    return result.intersection_size == 5 && 
           result.sender_set_size == 10 && 
           result.receiver_set_size == 10;
}

int main() {
    std::cout << "Secret Sharing PSI Protocol Test Suite" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // Item类测试
    std::cout << "\n--- Item Class Tests ---" << std::endl;
    TestRunner::run_test("Item Basic Operations", test_item_basic);
    TestRunner::run_test("Item Hash Function", test_item_hash);
    TestRunner::run_test("Item Value Conversion", test_item_value);
    
    // SecretSharing类测试
    std::cout << "\n--- Secret Sharing Tests ---" << std::endl;
    TestRunner::run_test("Seed Generation", test_secret_sharing_seeds);
    TestRunner::run_test("Triple Generation", test_secret_sharing_triples);
    TestRunner::run_test("Party Initialization", test_secret_sharing_initialization);
    
    // OPRF类测试
    std::cout << "\n--- OPRF Tests ---" << std::endl;
    TestRunner::run_test("OPRF Basic Operations", test_oprf_basic);
    TestRunner::run_test("OPRF Process Item", test_oprf_process_item);
    TestRunner::run_test("OPRF Batch Process", test_oprf_batch_process);
    
    // HashTable类测试
    std::cout << "\n--- Hash Table Tests ---" << std::endl;
    TestRunner::run_test("Hash Table Basic", test_hash_table_basic);
    TestRunner::run_test("Hash Table Batch Insert", test_hash_table_batch);
    
    // CuckooHashTable类测试
    std::cout << "\n--- Cuckoo Hash Table Tests ---" << std::endl;
    TestRunner::run_test("Cuckoo Table Basic", test_cuckoo_table_basic);
    TestRunner::run_test("Cuckoo Table Batch Insert", test_cuckoo_table_batch);
    
    // PSIProtocol类测试
    std::cout << "\n--- PSI Protocol Tests ---" << std::endl;
    TestRunner::run_test("Protocol Initialization", test_psi_protocol_initialization);
    TestRunner::run_test("Seed Generation", test_psi_protocol_seed_generation);
    TestRunner::run_test("Input Set Management", test_psi_protocol_input_set);
    TestRunner::run_test("Complete PSI Protocol", test_psi_protocol_complete);
    
    TestRunner::print_summary();
    
    return TestRunner::all_passed() ? 0 : 1;
}