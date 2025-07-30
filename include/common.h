#pragma once

#include <vector>
#include <array>
#include <cstdint>
#include <memory>
#include <random>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/sha.h>

namespace SecretSharingPSI {

// 基本数据类型
using uint128_t = __uint128_t;
using Seed = std::array<uint8_t, 32>;
using Hash = std::array<uint8_t, 32>;

// 海狸三元组结构
struct BeaverTriple {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    
    BeaverTriple() : a(0), b(0), c(0) {}
    BeaverTriple(uint64_t a_val, uint64_t b_val, uint64_t c_val) 
        : a(a_val), b(b_val), c(c_val) {}
};

// 二元组结构
struct BeaverPair {
    uint64_t a;
    uint64_t b;
    
    BeaverPair() : a(0), b(0) {}
    BeaverPair(uint64_t a_val, uint64_t b_val) : a(a_val), b(b_val) {}
};

// 秘密分享值
struct SecretShare {
    uint64_t share0;  // 参与方0的分享
    uint64_t share1;  // 参与方1的分享
    
    SecretShare() : share0(0), share1(0) {}
    SecretShare(uint64_t s0, uint64_t s1) : share0(s0), share1(s1) {}
    
    // 重构秘密值
    uint64_t reconstruct() const {
        return share0 + share1;
    }
};

// 参与方类型
enum class PartyType {
    PARTY_A = 0,
    PARTY_B = 1,
    TEE = 2
};

// 常量定义
constexpr size_t HASH_SIZE = 32;
constexpr size_t SEED_SIZE = 32;
constexpr size_t DEFAULT_TRIPLE_COUNT = 1000;
constexpr uint64_t FIELD_PRIME = (1ULL << 61) - 1;  // 2^61 - 1

// 工具函数
inline uint64_t mod_reduce(uint64_t x) {
    return x % FIELD_PRIME;
}

inline uint64_t mod_add(uint64_t a, uint64_t b) {
    return mod_reduce(a + b);
}

inline uint64_t mod_mul(uint64_t a, uint64_t b) {
    return mod_reduce((__uint128_t)a * b);
}

inline uint64_t mod_sub(uint64_t a, uint64_t b) {
    return mod_reduce(a + FIELD_PRIME - b);
}

} // namespace SecretSharingPSI