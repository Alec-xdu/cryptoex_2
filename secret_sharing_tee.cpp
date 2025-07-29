#include "secret_sharing_tee.h"
#include <iostream>
#include <cassert>
#include <openssl/evp.h>

SecretSharingTEE::SecretSharingTEE() {
    // 初始化OpenSSL随机数生成器
    if (RAND_status() != 1) {
        std::cerr << "OpenSSL random number generator not properly seeded" << std::endl;
        exit(1);
    }
    
    // 使用硬件随机数初始化C++随机数生成器
    std::random_device rd;
    rng_.seed(rd());
}

SecretSharingTEE::~SecretSharingTEE() {
    // 清理工作
}

std::vector<uint8_t> SecretSharingTEE::generateSecureSeed(size_t seed_length) {
    std::vector<uint8_t> seed(seed_length);
    
    // 使用OpenSSL生成加密安全的随机种子
    if (RAND_bytes(seed.data(), seed_length) != 1) {
        std::cerr << "Failed to generate secure random seed" << std::endl;
        exit(1);
    }
    
    return seed;
}

std::mt19937_64 SecretSharingTEE::createRNGFromSeed(const std::vector<uint8_t>& seed) {
    // 使用SHA-256哈希种子来创建64位种子值
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(seed.data(), seed.size(), hash);
    
    // 将哈希值转换为64位种子
    uint64_t seed_value = 0;
    for (int i = 0; i < 8; ++i) {
        seed_value |= (static_cast<uint64_t>(hash[i]) << (i * 8));
    }
    
    return std::mt19937_64(seed_value);
}

uint64_t SecretSharingTEE::generateRandomModP(std::mt19937_64& rng) {
    std::uniform_int_distribution<uint64_t> dist(0, PRIME_MODULUS - 1);
    return dist(rng);
}

void SecretSharingTEE::generateAndDistributeShares(size_t num_triplets,
                                                 PartyAData& party_a_data,
                                                 PartyBData& party_b_data) {
    // 生成种子
    party_a_data.seed_z = generateSecureSeed();
    party_b_data.seed_w = generateSecureSeed();
    
    // 基于种子创建RNG
    auto rng_a = createRNGFromSeed(party_a_data.seed_z);
    auto rng_b = createRNGFromSeed(party_b_data.seed_w);
    
    // 清空之前的数据
    party_a_data.triplets.clear();
    party_b_data.pairs.clear();
    party_b_data.c1_values.clear();
    
    // 预分配空间
    party_a_data.triplets.reserve(num_triplets);
    party_b_data.pairs.reserve(num_triplets);
    party_b_data.c1_values.reserve(num_triplets);
    
    for (size_t i = 0; i < num_triplets; ++i) {
        // A方生成三元组 [a0, b0, c0]
        uint64_t a0 = generateRandomModP(rng_a);
        uint64_t b0 = generateRandomModP(rng_a);
        uint64_t c0 = (a0 * b0) % PRIME_MODULUS;
        
        party_a_data.triplets.emplace_back(a0, b0, c0);
        
        // B方生成二元组 [a1, b1]
        uint64_t a1 = generateRandomModP(rng_b);
        uint64_t b1 = generateRandomModP(rng_b);
        
        party_b_data.pairs.emplace_back(a1, b1);
        
        // TEE计算 c1_i = (a0_i + a1_i) * (b0_i + b1_i) - c0_i
        uint64_t sum_a = (a0 + a1) % PRIME_MODULUS;
        uint64_t sum_b = (b0 + b1) % PRIME_MODULUS;
        uint64_t product = (sum_a * sum_b) % PRIME_MODULUS;
        uint64_t c1 = (product - c0 + PRIME_MODULUS) % PRIME_MODULUS;
        
        party_b_data.c1_values.push_back(c1);
    }
    
    std::cout << "Generated " << num_triplets << " Beaver triplets and distributed to both parties" << std::endl;
}

bool SecretSharingTEE::verifyBeaverTriplets(const std::vector<BeaverTriplet>& triplets_a,
                                          const std::vector<BeaverPair>& pairs_b,
                                          const std::vector<uint64_t>& c1_values) {
    if (triplets_a.size() != pairs_b.size() || pairs_b.size() != c1_values.size()) {
        std::cerr << "Size mismatch in Beaver triplet verification" << std::endl;
        return false;
    }
    
    for (size_t i = 0; i < triplets_a.size(); ++i) {
        const auto& triplet = triplets_a[i];
        const auto& pair = pairs_b[i];
        uint64_t c1 = c1_values[i];
        
        // 验证: c0 + c1 = (a0 + a1) * (b0 + b1) mod p
        uint64_t sum_a = (triplet.a + pair.a) % PRIME_MODULUS;
        uint64_t sum_b = (triplet.b + pair.b) % PRIME_MODULUS;
        uint64_t expected_sum_c = (sum_a * sum_b) % PRIME_MODULUS;
        uint64_t actual_sum_c = (triplet.c + c1) % PRIME_MODULUS;
        
        if (expected_sum_c != actual_sum_c) {
            std::cerr << "Beaver triplet verification failed at index " << i << std::endl;
            return false;
        }
    }
    
    std::cout << "All Beaver triplets verified successfully" << std::endl;
    return true;
}