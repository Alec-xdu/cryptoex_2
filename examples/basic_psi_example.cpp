#include "secret_sharing_psi.h"
#include <iostream>
#include <vector>
#include <string>
#include <chrono>

int main() {
    std::cout << "=== 基于秘密切片共享的PSI协议示例 ===" << std::endl;
    
    // 配置参数
    SecretSharingPSIParams params;
    params.sender_set_size = 1000;
    params.receiver_set_size = 100;
    params.bin_bundle_size = 64;
    params.num_hash_functions = 3;
    params.beaver_triplet_count = 5000;
    params.use_labeled_mode = false;
    
    std::cout << "参数配置:" << std::endl;
    std::cout << "  Sender集合大小: " << params.sender_set_size << std::endl;
    std::cout << "  Receiver集合大小: " << params.receiver_set_size << std::endl;
    std::cout << "  Bin bundle大小: " << params.bin_bundle_size << std::endl;
    std::cout << "  哈希函数数量: " << params.num_hash_functions << std::endl;
    std::cout << "  Beaver三元组数量: " << params.beaver_triplet_count << std::endl;
    
    // 创建PSI协议实例
    SecretSharingPSI psi(params);
    
    // 初始化协议
    std::cout << "\n正在初始化协议..." << std::endl;
    if (!psi.initialize()) {
        std::cerr << "协议初始化失败!" << std::endl;
        return 1;
    }
    std::cout << "协议初始化成功!" << std::endl;
    
    // 生成测试数据
    std::cout << "\n生成测试数据..." << std::endl;
    
    // Sender的数据集
    std::vector<std::string> sender_items;
    for (size_t i = 0; i < 50; ++i) {
        sender_items.push_back("sender_item_" + std::to_string(i));
    }
    
    // Receiver的查询集（包含一些与sender重叠的项目）
    std::vector<std::string> receiver_items;
    for (size_t i = 0; i < 10; ++i) {
        receiver_items.push_back("receiver_item_" + std::to_string(i));
    }
    
    // 添加一些重叠的项目
    for (size_t i = 20; i < 30; ++i) {
        receiver_items.push_back("sender_item_" + std::to_string(i));
    }
    
    std::cout << "  Sender数据: " << sender_items.size() << " 项目" << std::endl;
    std::cout << "  Receiver查询: " << receiver_items.size() << " 项目" << std::endl;
    std::cout << "  预期匹配数量: 10 项目" << std::endl;
    
    // 运行PSI协议
    std::cout << "\n执行PSI协议..." << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    PSIResult result = psi.runPSI(sender_items, receiver_items);
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // 显示结果
    if (result.success) {
        std::cout << "PSI协议执行成功!" << std::endl;
        std::cout << "执行时间: " << duration.count() << " ms" << std::endl;
        
        size_t match_count = 0;
        std::cout << "\n匹配结果:" << std::endl;
        for (size_t i = 0; i < result.match_results.size() && i < receiver_items.size(); ++i) {
            if (result.match_results[i]) {
                std::cout << "  ✓ " << receiver_items[i] << " (匹配)" << std::endl;
                match_count++;
            } else {
                std::cout << "  ✗ " << receiver_items[i] << " (不匹配)" << std::endl;
            }
        }
        
        std::cout << "\n总匹配数量: " << match_count << "/" << receiver_items.size() << std::endl;
        
        // 验证结果正确性
        size_t expected_matches = 0;
        for (const auto& receiver_item : receiver_items) {
            for (const auto& sender_item : sender_items) {
                if (receiver_item == sender_item) {
                    expected_matches++;
                    break;
                }
            }
        }
        
        if (match_count == expected_matches) {
            std::cout << "✓ 结果验证通过!" << std::endl;
        } else {
            std::cout << "✗ 结果验证失败! 预期: " << expected_matches << ", 实际: " << match_count << std::endl;
        }
        
    } else {
        std::cerr << "PSI协议执行失败!" << std::endl;
        return 1;
    }
    
    std::cout << "\n=== 示例程序完成 ===" << std::endl;
    return 0;
}