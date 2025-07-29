#include "../include/secret_share_distributor.h"
#include "../include/beaver_triple.h"
#include <iostream>
#include <vector>
#include <iomanip>
#include <chrono>

using namespace SecretShare;

void demonstrateBasicFunctionality() {
    std::cout << "=== 基本功能演示 ===" << std::endl;
    
    // 创建秘密分享分发器
    SecretShareDistributor distributor(32);
    
    // 1. 生成和分发种子
    Seed seed_z, seed_w;
    if (!distributor.generateAndDistributeSeeds(seed_z, seed_w)) {
        std::cerr << "Failed to generate seeds!" << std::endl;
        return;
    }
    
    std::cout << "种子生成成功:" << std::endl;
    std::cout << "  A方种子长度: " << seed_z.size() << " bytes" << std::endl;
    std::cout << "  B方种子长度: " << seed_w.size() << " bytes" << std::endl;
    
    // 2. 生成Beaver Triple
    const size_t triple_count = 5;
    
    auto triples_a = distributor.generateTriplesForPartyA(seed_z, triple_count);
    auto triples_b = distributor.generateTriplesForPartyB(seed_w, seed_z, triple_count);
    
    std::cout << "\n生成了 " << triple_count << " 个Beaver Triple:" << std::endl;
    
    // 3. 验证Beaver Triple的正确性
    bool verification_passed = BeaverTripleUtils::verifyBatchTriples(triples_a, triples_b);
    std::cout << "Beaver Triple验证: " << (verification_passed ? "通过" : "失败") << std::endl;
    
    // 打印前3个三元组
    for (size_t i = 0; i < std::min(size_t(3), triple_count); ++i) {
        std::cout << "\n三元组 " << i << ":" << std::endl;
        BeaverTripleUtils::printTriple(triples_a[i], "A方");
        BeaverTripleUtils::printTriple(triples_b[i], "B方");
        
        // 重建原始值并验证
        Share a = triples_a[i].a + triples_b[i].a;
        Share b = triples_a[i].b + triples_b[i].b;
        Share c = triples_a[i].c + triples_b[i].c;
        std::cout << "  重建值: a=" << a << ", b=" << b << ", c=" << c 
                  << ", a*b=" << (a * b) << std::endl;
    }
}

void demonstrateMultiplication() {
    std::cout << "\n\n=== Beaver Triple乘法运算演示 ===" << std::endl;
    
    SecretShareDistributor distributor(32);
    
    // 生成种子和三元组
    Seed seed_z, seed_w;
    distributor.generateAndDistributeSeeds(seed_z, seed_w);
    
    const size_t operation_count = 3;
    auto triples_a = distributor.generateTriplesForPartyA(seed_z, operation_count);
    auto triples_b = distributor.generateTriplesForPartyB(seed_w, seed_z, operation_count);
    
    // 准备测试数据
    std::vector<Share> x_values = {12, 25, 7};
    std::vector<Share> y_values = {8, 4, 15};
    
    std::vector<Share> x_shares_a, x_shares_b;
    std::vector<Share> y_shares_a, y_shares_b;
    
    BeaverTripleUtils::generateBatchTestShares(x_values, x_shares_a, x_shares_b);
    BeaverTripleUtils::generateBatchTestShares(y_values, y_shares_a, y_shares_b);
    
    std::cout << "测试数据:" << std::endl;
    for (size_t i = 0; i < operation_count; ++i) {
        std::cout << "  x[" << i << "] = " << x_values[i] 
                  << ", y[" << i << "] = " << y_values[i]
                  << ", 期望结果 = " << (x_values[i] * y_values[i]) << std::endl;
    }
    
    // 执行两方乘法运算
    std::cout << "\n乘法运算结果:" << std::endl;
    for (size_t i = 0; i < operation_count; ++i) {
        Share result = distributor.twoPartyMultiplyWithBeaverTriple(
            x_shares_a[i], x_shares_b[i], y_shares_a[i], y_shares_b[i], 
            triples_a[i], triples_b[i]);
        Share expected = x_values[i] * y_values[i];
        
        std::cout << "  结果[" << i << "] = " << result 
                  << " (期望: " << expected 
                  << ", 正确: " << (result == expected ? "是" : "否") << ")" << std::endl;
    }
}

void demonstratePerformance() {
    std::cout << "\n\n=== 性能测试 ===" << std::endl;
    
    SecretShareDistributor distributor(32);
    
    const std::vector<size_t> test_sizes = {100, 1000, 10000};
    
    for (size_t size : test_sizes) {
        std::cout << "\n测试规模: " << size << " 个三元组" << std::endl;
        
        // 测试种子生成时间
        auto start = std::chrono::high_resolution_clock::now();
        
        Seed seed_z, seed_w;
        distributor.generateAndDistributeSeeds(seed_z, seed_w);
        
        auto seed_end = std::chrono::high_resolution_clock::now();
        
        // 测试三元组生成时间
        auto triples_a = distributor.generateTriplesForPartyA(seed_z, size);
        auto triple_a_end = std::chrono::high_resolution_clock::now();
        
        auto triples_b = distributor.generateTriplesForPartyB(seed_w, seed_z, size);
        auto triple_b_end = std::chrono::high_resolution_clock::now();
        
        // 计算时间
        auto seed_time = std::chrono::duration_cast<std::chrono::microseconds>(seed_end - start);
        auto triple_a_time = std::chrono::duration_cast<std::chrono::microseconds>(triple_a_end - seed_end);
        auto triple_b_time = std::chrono::duration_cast<std::chrono::microseconds>(triple_b_end - triple_a_end);
        
        std::cout << "  种子生成时间: " << seed_time.count() << " μs" << std::endl;
        std::cout << "  A方三元组生成时间: " << triple_a_time.count() << " μs" << std::endl;
        std::cout << "  B方三元组生成时间: " << triple_b_time.count() << " μs" << std::endl;
        std::cout << "  平均每个三元组: " << 
                     (triple_a_time.count() + triple_b_time.count()) / (2.0 * size) << " μs" << std::endl;
        
        // 验证一小部分三元组
        const size_t verify_count = std::min(size_t(10), size);
        std::vector<BeaverTriple> sample_a(triples_a.begin(), triples_a.begin() + verify_count);
        std::vector<BeaverTriple> sample_b(triples_b.begin(), triples_b.begin() + verify_count);
        
        bool verification_passed = BeaverTripleUtils::verifyBatchTriples(sample_a, sample_b);
        std::cout << "  验证结果: " << (verification_passed ? "通过" : "失败") << std::endl;
    }
}

#ifdef HAVE_APSI
void demonstrateAPSIIntegration() {
    std::cout << "\n\n=== APSI集成演示 ===" << std::endl;
    
    SecretShareDistributor distributor(32);
    
    // 创建测试APSI Items
    std::vector<ItemType> test_items;
    for (int i = 0; i < 5; ++i) {
        std::vector<uint8_t> data(8);
        uint64_t value = 100 + i * 10;
        std::memcpy(data.data(), &value, sizeof(value));
        test_items.push_back(ItemType(data));
    }
    
    std::cout << "原始APSI Items:" << std::endl;
    auto shares = distributor.itemsToShares(test_items);
    for (size_t i = 0; i < shares.size(); ++i) {
        std::cout << "  Item[" << i << "] -> Share: " << shares[i] << std::endl;
    }
    
    // 转换回APSI Items
    auto converted_items = distributor.sharesToItems(shares);
    auto converted_shares = distributor.itemsToShares(converted_items);
    
    std::cout << "\n转换验证:" << std::endl;
    bool conversion_correct = true;
    for (size_t i = 0; i < shares.size(); ++i) {
        bool match = (shares[i] == converted_shares[i]);
        conversion_correct &= match;
        std::cout << "  Item[" << i << "] 转换: " << (match ? "正确" : "错误") << std::endl;
    }
    
    std::cout << "整体转换结果: " << (conversion_correct ? "成功" : "失败") << std::endl;
}
#endif

int main() {
    std::cout << "秘密切片分发和Beaver Triple乘法运算演示程序" << std::endl;
    std::cout << "=============================================" << std::endl;
    
    try {
        demonstrateBasicFunctionality();
        demonstrateMultiplication();
        demonstratePerformance();
        
#ifdef HAVE_APSI
        demonstrateAPSIIntegration();
#else
        std::cout << "\n注意: APSI库未找到，跳过APSI集成演示" << std::endl;
#endif
        
        std::cout << "\n\n所有演示完成！" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}