#ifndef SECRET_SHARING_TEE_H
#define SECRET_SHARING_TEE_H

#include <vector>
#include <cstdint>
#include <random>
#include <openssl/rand.h>
#include <openssl/sha.h>

// Beaver三元组结构
struct BeaverTriplet {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    
    BeaverTriplet() : a(0), b(0), c(0) {}
    BeaverTriplet(uint64_t a_val, uint64_t b_val, uint64_t c_val) 
        : a(a_val), b(b_val), c(c_val) {}
};

// 二元组结构（用于B方）
struct BeaverPair {
    uint64_t a;
    uint64_t b;
    
    BeaverPair() : a(0), b(0) {}
    BeaverPair(uint64_t a_val, uint64_t b_val) : a(a_val), b(b_val) {}
};

// 秘密切片共享TEE类
class SecretSharingTEE {
private:
    static const uint64_t PRIME_MODULUS = 2147483647; // 2^31 - 1, 大素数
    std::mt19937_64 rng_;
    
    // 生成安全随机数种子
    std::vector<uint8_t> generateSecureSeed(size_t seed_length = 32);
    
    // 基于种子生成伪随机数生成器
    std::mt19937_64 createRNGFromSeed(const std::vector<uint8_t>& seed);
    
    // 生成模素数的随机数
    uint64_t generateRandomModP(std::mt19937_64& rng);

public:
    SecretSharingTEE();
    ~SecretSharingTEE();
    
    // 为A方生成种子和Beaver三元组
    struct PartyAData {
        std::vector<uint8_t> seed_z;
        std::vector<BeaverTriplet> triplets;
    };
    
    // 为B方生成种子和二元组
    struct PartyBData {
        std::vector<uint8_t> seed_w;
        std::vector<BeaverPair> pairs;
        std::vector<uint64_t> c1_values; // c1_i = (a0_i + a1_i)*(b0_i + b1_i) - c0_i
    };
    
    // 生成并分发秘密切片给双方
    void generateAndDistributeShares(size_t num_triplets, 
                                   PartyAData& party_a_data,
                                   PartyBData& party_b_data);
    
    // 验证Beaver三元组的正确性
    bool verifyBeaverTriplets(const std::vector<BeaverTriplet>& triplets_a,
                             const std::vector<BeaverPair>& pairs_b,
                             const std::vector<uint64_t>& c1_values);
    
    // 获取素数模数
    static uint64_t getPrimeModulus() { return PRIME_MODULUS; }
};

#endif // SECRET_SHARING_TEE_H