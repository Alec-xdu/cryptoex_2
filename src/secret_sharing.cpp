#include "secret_sharing.h"
#include <stdexcept>
#include <cassert>
#include <iostream>

namespace SecretSharingPSI {

SecretSharing::SecretSharing() : party_type_(PartyType::TEE) {
    // 初始化随机数生成器
    Seed random_seed = generateRandomSeed();
    rng_ = createPRNG(random_seed);
}

SecretSharing::~SecretSharing() = default;

bool SecretSharing::generateSeeds(Seed& seed_a, Seed& seed_b) {
    try {
        seed_a = generateRandomSeed();
        seed_b = generateRandomSeed();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error generating seeds: " << e.what() << std::endl;
        return false;
    }
}

std::vector<BeaverTriple> SecretSharing::generateTriplesForA(const Seed& seed_a, size_t count) {
    std::vector<BeaverTriple> triples;
    triples.reserve(count);
    
    // 使用参与方A的种子生成确定性的三元组
    auto rng_a = createPRNG(seed_a);
    
    for (size_t i = 0; i < count; ++i) {
        triples.push_back(generateRandomTriple(rng_a));
    }
    
    return triples;
}

SecretSharing::PartyBData SecretSharing::generateDataForB(
    const Seed& seed_a, const Seed& seed_b, size_t count) {
    
    PartyBData result;
    result.pairs.reserve(count);
    result.c1_values.reserve(count);
    
    // 使用参与方A的种子重新生成A的三元组
    auto rng_a = createPRNG(seed_a);
    // 使用参与方B的种子生成B的二元组
    auto rng_b = createPRNG(seed_b);
    
    for (size_t i = 0; i < count; ++i) {
        // 生成A的三元组 [a0, b0, c0]
        BeaverTriple triple_a = generateRandomTriple(rng_a);
        
        // 生成B的二元组 [a1, b1]
        BeaverPair pair_b = generateRandomPair(rng_b);
        
        // 计算 c1 = (a0 + a1) * (b0 + b1) - c0
        uint64_t a_sum = mod_add(triple_a.a, pair_b.a);
        uint64_t b_sum = mod_add(triple_a.b, pair_b.b);
        uint64_t product = mod_mul(a_sum, b_sum);
        uint64_t c1 = mod_sub(product, triple_a.c);
        
        result.pairs.push_back(pair_b);
        result.c1_values.push_back(c1);
    }
    
    return result;
}

void SecretSharing::initializePartyA(const Seed& seed, size_t count) {
    party_type_ = PartyType::PARTY_A;
    current_seed_ = seed;
    
    // 使用种子生成本地三元组
    triples_a_ = generateTriplesForA(seed, count);
    
    // 初始化随机数生成器
    rng_ = createPRNG(seed);
}

BeaverTriple SecretSharing::getTripleA(size_t index) const {
    validateTripleIndex(index);
    return triples_a_[index];
}

void SecretSharing::initializePartyB(const Seed& seed, 
                                   const std::vector<BeaverPair>& pairs,
                                   const std::vector<uint64_t>& c1_values) {
    party_type_ = PartyType::PARTY_B;
    current_seed_ = seed;
    pairs_b_ = pairs;
    c1_values_b_ = c1_values;
    
    // 初始化随机数生成器
    rng_ = createPRNG(seed);
}

SecretSharing::PartyBTriple SecretSharing::getDataB(size_t index) const {
    if (index >= pairs_b_.size() || index >= c1_values_b_.size()) {
        throw std::out_of_range("Invalid triple index for party B");
    }
    
    PartyBTriple result;
    result.pair = pairs_b_[index];
    result.c1 = c1_values_b_[index];
    return result;
}

uint64_t SecretSharing::secureMultiplyA(uint64_t x_share_a, uint64_t y_share_a, size_t triple_index) {
    if (party_type_ != PartyType::PARTY_A) {
        throw std::runtime_error("This method can only be called by Party A");
    }
    
    validateTripleIndex(triple_index);
    BeaverTriple triple = triples_a_[triple_index];
    
    // 计算 d_a = x_a - a0, e_a = y_a - b0
    uint64_t d_a = mod_sub(x_share_a, triple.a);
    uint64_t e_a = mod_sub(y_share_a, triple.b);
    
    // 注意：在实际协议中，这里需要与参与方B通信交换d和e的值
    // 为了演示，我们假设已经获得了完整的d和e值
    // 在实际实现中，需要添加网络通信层
    
    // 临时假设d和e的完整值（实际中需要从通信中获得）
    uint64_t d = d_a;  // 实际中需要 d_a + d_b
    uint64_t e = e_a;  // 实际中需要 e_a + e_b
    
    // 计算A的乘法结果分享：z_a = c0 + d*b0 + e*a0 + d*e
    uint64_t result = triple.c;
    result = mod_add(result, mod_mul(d, triple.b));
    result = mod_add(result, mod_mul(e, triple.a));
    result = mod_add(result, mod_mul(d, e));
    
    return result;
}

uint64_t SecretSharing::secureMultiplyB(uint64_t x_share_b, uint64_t y_share_b, size_t triple_index) {
    if (party_type_ != PartyType::PARTY_B) {
        throw std::runtime_error("This method can only be called by Party B");
    }
    
    if (triple_index >= pairs_b_.size()) {
        throw std::out_of_range("Invalid triple index for party B");
    }
    
    PartyBTriple data = getDataB(triple_index);
    
    // 计算 d_b = x_b - a1, e_b = y_b - b1
    uint64_t d_b = mod_sub(x_share_b, data.pair.a);
    uint64_t e_b = mod_sub(y_share_b, data.pair.b);
    
    // 注意：在实际协议中，这里需要与参与方A通信交换d和e的值
    // 为了演示，我们假设已经获得了完整的d和e值
    // 在实际实现中，需要添加网络通信层
    
    // 临时假设d和e的完整值（实际中需要从通信中获得）
    uint64_t d = d_b;  // 实际中需要 d_a + d_b
    uint64_t e = e_b;  // 实际中需要 e_a + e_b
    
    // 计算B的乘法结果分享：z_b = c1 + d*b1 + e*a1
    uint64_t result = data.c1;
    result = mod_add(result, mod_mul(d, data.pair.b));
    result = mod_add(result, mod_mul(e, data.pair.a));
    
    return result;
}

Seed SecretSharing::generateRandomSeed() {
    Seed seed;
    if (RAND_bytes(seed.data(), SEED_SIZE) != 1) {
        throw std::runtime_error("Failed to generate random seed");
    }
    return seed;
}

std::mt19937_64 SecretSharing::createPRNG(const Seed& seed) {
    // 将种子转换为64位整数来初始化随机数生成器
    uint64_t seed_value = 0;
    for (size_t i = 0; i < 8 && i < seed.size(); ++i) {
        seed_value |= (static_cast<uint64_t>(seed[i]) << (i * 8));
    }
    return std::mt19937_64(seed_value);
}

uint64_t SecretSharing::generateRandom64(std::mt19937_64& rng) {
    return mod_reduce(rng());
}

void SecretSharing::validateTripleIndex(size_t index) const {
    if (party_type_ == PartyType::PARTY_A && index >= triples_a_.size()) {
        throw std::out_of_range("Invalid triple index for party A");
    }
}

BeaverTriple SecretSharing::generateRandomTriple(std::mt19937_64& rng) {
    uint64_t a = generateRandom64(rng);
    uint64_t b = generateRandom64(rng);
    uint64_t c = mod_mul(a, b);  // c = a * b
    return BeaverTriple(a, b, c);
}

BeaverPair SecretSharing::generateRandomPair(std::mt19937_64& rng) {
    uint64_t a = generateRandom64(rng);
    uint64_t b = generateRandom64(rng);
    return BeaverPair(a, b);
}

} // namespace SecretSharingPSI