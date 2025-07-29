#include "secret_sharing_tee.h"
#include "arithmetic_secret_sharing.h"
#include <iostream>
#include <vector>
#include <random>
#include <chrono>

int main() {
    std::cout << "=== Beaver三元组和秘密切片共享测试 ===" << std::endl;
    
    // 创建TEE实例
    auto tee = std::make_shared<SecretSharingTEE>();
    std::cout << "TEE实例创建成功" << std::endl;
    
    // 测试Beaver三元组生成
    std::cout << "\n--- 测试Beaver三元组生成 ---" << std::endl;
    
    size_t num_triplets = 1000;
    SecretSharingTEE::PartyAData party_a_data;
    SecretSharingTEE::PartyBData party_b_data;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    tee->generateAndDistributeShares(num_triplets, party_a_data, party_b_data);
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "生成 " << num_triplets << " 个Beaver三元组用时: " << duration.count() << " μs" << std::endl;
    
    // 验证Beaver三元组
    std::cout << "\n验证Beaver三元组正确性..." << std::endl;
    bool verification_result = tee->verifyBeaverTriplets(
        party_a_data.triplets, 
        party_b_data.pairs, 
        party_b_data.c1_values
    );
    
    if (verification_result) {
        std::cout << "✓ Beaver三元组验证通过!" << std::endl;
    } else {
        std::cout << "✗ Beaver三元组验证失败!" << std::endl;
        return 1;
    }
    
    // 测试算数秘密共享运算
    std::cout << "\n--- 测试算数秘密共享运算 ---" << std::endl;
    
    // 创建算术引擎
    auto arithmetic_engine = std::make_shared<ArithmeticSecretSharing>(tee, true);
    
    // 初始化算术引擎
    if (!arithmetic_engine->initialize(num_triplets)) {
        std::cerr << "算术引擎初始化失败!" << std::endl;
        return 1;
    }
    std::cout << "算术引擎初始化成功" << std::endl;
    
    // 测试基本运算
    std::cout << "\n测试基本运算..." << std::endl;
    
    // 测试加法
    std::cout << "测试秘密共享加法:" << std::endl;
    uint64_t a = 123456;
    uint64_t b = 789012;
    
    auto share_a = arithmetic_engine->createShare(a);
    auto share_b = arithmetic_engine->createShare(b);
    auto share_sum = arithmetic_engine->addShares(share_a, share_b);
    
    uint64_t reconstructed_sum = arithmetic_engine->reconstructSecret(share_sum);
    uint64_t expected_sum = (a + b) % SecretSharingTEE::getPrimeModulus();
    
    std::cout << "  " << a << " + " << b << " = " << reconstructed_sum << std::endl;
    std::cout << "  预期结果: " << expected_sum << std::endl;
    
    if (reconstructed_sum == expected_sum) {
        std::cout << "  ✓ 加法测试通过!" << std::endl;
    } else {
        std::cout << "  ✗ 加法测试失败!" << std::endl;
    }
    
    // 测试乘法
    std::cout << "\n测试秘密共享乘法:" << std::endl;
    uint64_t x = 12345;
    uint64_t y = 67890;
    
    auto share_x = arithmetic_engine->createShare(x);
    auto share_y = arithmetic_engine->createShare(y);
    auto share_product = arithmetic_engine->mulShares(share_x, share_y);
    
    uint64_t reconstructed_product = arithmetic_engine->reconstructSecret(share_product);
    uint64_t expected_product = (x * y) % SecretSharingTEE::getPrimeModulus();
    
    std::cout << "  " << x << " * " << y << " = " << reconstructed_product << std::endl;
    std::cout << "  预期结果: " << expected_product << std::endl;
    
    if (reconstructed_product == expected_product) {
        std::cout << "  ✓ 乘法测试通过!" << std::endl;
    } else {
        std::cout << "  ✗ 乘法测试失败!" << std::endl;
    }
    
    // 测试多项式计算
    std::cout << "\n--- 测试多项式计算 ---" << std::endl;
    
    // 创建一个简单的多项式匹配场景
    std::vector<uint64_t> sender_items = {1001, 2002, 3003, 4004, 5005};
    uint64_t receiver_query = 3003;  // 这个应该匹配
    
    auto query_share = arithmetic_engine->createShare(receiver_query);
    
    std::cout << "Sender项目: ";
    for (auto item : sender_items) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
    std::cout << "Receiver查询: " << receiver_query << std::endl;
    
    // 计算匹配多项式
    start_time = std::chrono::high_resolution_clock::now();
    auto poly_result = arithmetic_engine->computeMatchingPolynomial(sender_items, query_share);
    end_time = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    std::cout << "多项式计算用时: " << duration.count() << " μs" << std::endl;
    
    if (poly_result.success && !poly_result.evaluation_shares.empty()) {
        uint64_t result_value = arithmetic_engine->reconstructSecret(poly_result.evaluation_shares[0]);
        std::cout << "多项式求值结果: " << result_value << std::endl;
        
        if (result_value == 0) {
            std::cout << "✓ 匹配成功! (多项式值为0)" << std::endl;
        } else {
            std::cout << "✗ 不匹配 (多项式值为" << result_value << ")" << std::endl;
        }
    } else {
        std::cout << "✗ 多项式计算失败!" << std::endl;
    }
    
    // 测试不匹配的情况
    std::cout << "\n测试不匹配的情况..." << std::endl;
    uint64_t non_matching_query = 9999;
    auto non_matching_share = arithmetic_engine->createShare(non_matching_query);
    
    auto non_match_result = arithmetic_engine->computeMatchingPolynomial(sender_items, non_matching_share);
    
    if (non_match_result.success && !non_match_result.evaluation_shares.empty()) {
        uint64_t non_match_value = arithmetic_engine->reconstructSecret(non_match_result.evaluation_shares[0]);
        std::cout << "查询 " << non_matching_query << " 的多项式结果: " << non_match_value << std::endl;
        
        if (non_match_value != 0) {
            std::cout << "✓ 正确识别为不匹配!" << std::endl;
        } else {
            std::cout << "✗ 错误: 不应该匹配但结果为0" << std::endl;
        }
    }
    
    // 性能测试
    std::cout << "\n--- 性能测试 ---" << std::endl;
    
    size_t perf_iterations = 100;
    std::vector<uint64_t> perf_times;
    
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(1, 1000000);
    
    for (size_t i = 0; i < perf_iterations; ++i) {
        uint64_t val1 = dist(gen);
        uint64_t val2 = dist(gen);
        
        auto s1 = arithmetic_engine->createShare(val1);
        auto s2 = arithmetic_engine->createShare(val2);
        
        start_time = std::chrono::high_resolution_clock::now();
        auto product = arithmetic_engine->mulShares(s1, s2);
        end_time = std::chrono::high_resolution_clock::now();
        
        auto mul_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        perf_times.push_back(mul_time);
    }
    
    // 计算平均时间
    uint64_t total_time = 0;
    for (auto time : perf_times) {
        total_time += time;
    }
    double avg_time = static_cast<double>(total_time) / perf_iterations;
    
    std::cout << "秘密共享乘法性能 (" << perf_iterations << " 次测试):" << std::endl;
    std::cout << "  平均时间: " << avg_time << " ns" << std::endl;
    std::cout << "  剩余Beaver三元组: " << arithmetic_engine->getRemainingTriplets() << std::endl;
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    return 0;
}