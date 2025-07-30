#pragma once

#include "common.h"
#include <unordered_map>
#include <string>

namespace SecretSharingPSI {

/**
 * 秘密分享类 - 实现基于海狸三元组的秘密分享协议
 * TEE负责生成种子和预处理三元组/二元组
 * 参与方A和B使用种子生成本地分享并进行安全计算
 */
class SecretSharing {
public:
    SecretSharing();
    ~SecretSharing();

    // TEE相关功能
    /**
     * TEE生成种子对并分发给参与方
     * @param seed_a 发送给参与方A的种子
     * @param seed_b 发送给参与方B的种子
     * @return 是否成功生成种子
     */
    bool generateSeeds(Seed& seed_a, Seed& seed_b);

    /**
     * TEE预生成海狸三元组给参与方A
     * @param seed_a 参与方A的种子
     * @param count 三元组数量
     * @return 三元组向量
     */
    std::vector<BeaverTriple> generateTriplesForA(const Seed& seed_a, size_t count);

    /**
     * TEE预生成海狸二元组给参与方B，并计算对应的c1值
     * @param seed_a 参与方A的种子（用于获取A的三元组）
     * @param seed_b 参与方B的种子
     * @param count 二元组数量
     * @return 包含二元组和c1值的结构
     */
    struct PartyBData {
        std::vector<BeaverPair> pairs;
        std::vector<uint64_t> c1_values;
    };
    PartyBData generateDataForB(const Seed& seed_a, const Seed& seed_b, size_t count);

    // 参与方A的功能
    /**
     * 参与方A初始化，使用TEE提供的种子生成本地三元组
     * @param seed 从TEE接收的种子
     * @param count 三元组数量
     */
    void initializePartyA(const Seed& seed, size_t count);

    /**
     * 参与方A获取指定索引的三元组
     * @param index 三元组索引
     * @return 海狸三元组
     */
    BeaverTriple getTripleA(size_t index) const;

    // 参与方B的功能
    /**
     * 参与方B初始化，使用TEE提供的种子和数据
     * @param seed 从TEE接收的种子
     * @param pairs 从TEE接收的二元组
     * @param c1_values 从TEE计算的c1值
     */
    void initializePartyB(const Seed& seed, const std::vector<BeaverPair>& pairs, 
                         const std::vector<uint64_t>& c1_values);

    /**
     * 参与方B获取指定索引的数据
     * @param index 索引
     * @return 包含二元组和c1值的结构
     */
    struct PartyBTriple {
        BeaverPair pair;
        uint64_t c1;
    };
    PartyBTriple getDataB(size_t index) const;

    // 安全乘法协议
    /**
     * 基于海狸三元组的安全乘法 - 参与方A
     * @param x_share_a A持有的x的分享
     * @param y_share_a A持有的y的分享
     * @param triple_index 使用的三元组索引
     * @return A的乘法结果分享
     */
    uint64_t secureMultiplyA(uint64_t x_share_a, uint64_t y_share_a, size_t triple_index);

    /**
     * 基于海狸三元组的安全乘法 - 参与方B
     * @param x_share_b B持有的x的分享
     * @param y_share_b B持有的y的分享
     * @param triple_index 使用的三元组索引
     * @return B的乘法结果分享
     */
    uint64_t secureMultiplyB(uint64_t x_share_b, uint64_t y_share_b, size_t triple_index);

    // 工具函数
    /**
     * 生成随机种子
     * @return 32字节随机种子
     */
    static Seed generateRandomSeed();

    /**
     * 使用种子初始化伪随机数生成器
     * @param seed 种子
     * @return 初始化的随机数生成器
     */
    static std::mt19937_64 createPRNG(const Seed& seed);

    /**
     * 生成随机64位整数
     * @param rng 随机数生成器
     * @return 随机64位整数
     */
    static uint64_t generateRandom64(std::mt19937_64& rng);

private:
    // 当前参与方类型
    PartyType party_type_;
    
    // 参与方A的数据
    std::vector<BeaverTriple> triples_a_;
    
    // 参与方B的数据
    std::vector<BeaverPair> pairs_b_;
    std::vector<uint64_t> c1_values_b_;
    
    // 种子
    Seed current_seed_;
    
    // 随机数生成器
    std::mt19937_64 rng_;
    
    // 内部辅助函数
    void validateTripleIndex(size_t index) const;
    BeaverTriple generateRandomTriple(std::mt19937_64& rng);
    BeaverPair generateRandomPair(std::mt19937_64& rng);
};

} // namespace SecretSharingPSI