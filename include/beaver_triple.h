#pragma once

#include "secret_share_distributor.h"
#include <iostream>

namespace SecretShare {

/**
 * Beaver Triple工具类，提供辅助功能
 */
class BeaverTripleUtils {
public:
    /**
     * 打印Beaver Triple信息（用于调试）
     * @param triple Beaver Triple
     * @param party_name 方名称
     */
    static void printTriple(const BeaverTriple& triple, const std::string& party_name);
    
    /**
     * 验证Beaver Triple的正确性
     * @param triple_a A方的三元组
     * @param triple_b B方的三元组
     * @return 验证是否通过
     */
    static bool verifyTriples(const BeaverTriple& triple_a, const BeaverTriple& triple_b);
    
    /**
     * 批量验证Beaver Triple
     * @param triples_a A方的三元组向量
     * @param triples_b B方的三元组向量
     * @return 验证是否通过
     */
    static bool verifyBatchTriples(const std::vector<BeaverTriple>& triples_a, 
                                  const std::vector<BeaverTriple>& triples_b);
    
    /**
     * 生成测试用的秘密分享
     * @param secret 原始秘密值
     * @param share_a A方的分享（输出）
     * @param share_b B方的分享（输出）
     */
    static void generateTestShares(Share secret, Share& share_a, Share& share_b);
    
    /**
     * 批量生成测试用的秘密分享
     * @param secrets 原始秘密值向量
     * @param shares_a A方的分享向量（输出）
     * @param shares_b B方的分享向量（输出）
     */
    static void generateBatchTestShares(const std::vector<Share>& secrets,
                                       std::vector<Share>& shares_a,
                                       std::vector<Share>& shares_b);
};

} // namespace SecretShare