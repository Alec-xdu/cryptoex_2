#include "../include/secret_share_distributor.h"
#include "../include/rng_utils.h"
#include <stdexcept>
#include <cstring>

#ifdef HAVE_APSI
#include <apsi/util/utils.h>
#endif

namespace SecretShare {

SecretShareDistributor::SecretShareDistributor(size_t seed_length) 
    : seed_length_(seed_length), cipher_ctx_(nullptr) {
    if (seed_length_ < 16) {
        throw std::invalid_argument("Seed length must be at least 16 bytes for AES");
    }
}

SecretShareDistributor::~SecretShareDistributor() {
    if (cipher_ctx_) {
        RNGUtils::cleanupCipherContext(cipher_ctx_);
    }
}

bool SecretShareDistributor::generateAndDistributeSeeds(Seed& seed_z, Seed& seed_w) {
    try {
        // 生成给A方的种子z
        seed_z = RNGUtils::generateSecureRandomSeed(seed_length_);
        
        // 生成给B方的种子w
        seed_w = RNGUtils::generateSecureRandomSeed(seed_length_);
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

std::vector<BeaverTriple> SecretShareDistributor::generateTriplesForPartyA(const Seed& seed_z, size_t count) {
    std::vector<BeaverTriple> triples;
    triples.reserve(count);
    
    // 使用种子z初始化伪随机数生成器
    EVP_CIPHER_CTX* ctx = RNGUtils::createCipherContext(seed_z);
    if (!ctx) {
        throw std::runtime_error("Failed to initialize PRNG for party A");
    }
    
    try {
        for (size_t i = 0; i < count; ++i) {
            BeaverTriple triple;
            
            // 生成随机的a0, b0
            triple.a = RNGUtils::generatePseudoRandomUint64(ctx);
            triple.b = RNGUtils::generatePseudoRandomUint64(ctx);
            
            // c0 = a0 * b0
            triple.c = triple.a * triple.b;
            
            triples.push_back(triple);
        }
    } catch (...) {
        RNGUtils::cleanupCipherContext(ctx);
        throw;
    }
    
    RNGUtils::cleanupCipherContext(ctx);
    return triples;
}

std::vector<BeaverTriple> SecretShareDistributor::generateTriplesForPartyB(
    const Seed& seed_w, const Seed& seed_z, size_t count) {
    
    std::vector<BeaverTriple> triples;
    triples.reserve(count);
    
    // 使用种子w初始化B方的伪随机数生成器
    EVP_CIPHER_CTX* ctx_w = RNGUtils::createCipherContext(seed_w);
    if (!ctx_w) {
        throw std::runtime_error("Failed to initialize PRNG for party B");
    }
    
    // 使用种子z获取A方的三元组（用于计算c1）
    EVP_CIPHER_CTX* ctx_z = RNGUtils::createCipherContext(seed_z);
    if (!ctx_z) {
        RNGUtils::cleanupCipherContext(ctx_w);
        throw std::runtime_error("Failed to initialize PRNG for party A data");
    }
    
    try {
        for (size_t i = 0; i < count; ++i) {
            BeaverTriple triple;
            
            // 生成B方的随机数a1, b1
            triple.a = RNGUtils::generatePseudoRandomUint64(ctx_w);
            triple.b = RNGUtils::generatePseudoRandomUint64(ctx_w);
            
            // 获取A方对应的a0, b0, c0
            Share a0 = RNGUtils::generatePseudoRandomUint64(ctx_z);
            Share b0 = RNGUtils::generatePseudoRandomUint64(ctx_z);
            Share c0 = a0 * b0;
            
            // 计算c1 = (a0 + a1) * (b0 + b1) - c0
            triple.c = (a0 + triple.a) * (b0 + triple.b) - c0;
            
            triples.push_back(triple);
        }
    } catch (...) {
        RNGUtils::cleanupCipherContext(ctx_w);
        RNGUtils::cleanupCipherContext(ctx_z);
        throw;
    }
    
    RNGUtils::cleanupCipherContext(ctx_w);
    RNGUtils::cleanupCipherContext(ctx_z);
    return triples;
}

Share SecretShareDistributor::multiplyWithBeaverTriple(
    Share x_share, Share y_share, const BeaverTriple& triple, bool is_party_a) {
    
    // 标准Beaver Triple乘法协议：
    // 输入：x_share, y_share是x和y的秘密分享
    // triple包含预计算的(a,b,c)三元组，其中c = a*b
    // 目标：计算x*y的秘密分享
    
    // 第一步：计算d = x - a, e = y - b的分享
    Share d_share = x_share - triple.a;
    Share e_share = y_share - triple.b;
    
    // 在真实的两方协议中，这里需要重建d和e的完整值
    // 为了演示，我们假设能够访问对方的分享来重建完整值
    // 注意：这是简化的实现，实际应用中需要通过安全通信重建
    
    // 第二步：使用三元组计算结果分享
    // 公式：z = c + d*b + e*a + (d*e if party_a else 0)
    Share result = triple.c + d_share * triple.b + e_share * triple.a;
    
    if (is_party_a) {
        // 只有A方需要加上d*e项
        result += d_share * e_share;
    }
    
    return result;
}

// 新增：完整的两方Beaver Triple乘法协议
Share SecretShareDistributor::twoPartyMultiplyWithBeaverTriple(
    Share x_a, Share x_b, Share y_a, Share y_b, 
    const BeaverTriple& triple_a, const BeaverTriple& triple_b) {
    
    // 第一步：双方计算各自的d和e分享
    Share d_a = x_a - triple_a.a;
    Share d_b = x_b - triple_b.a;
    Share e_a = y_a - triple_a.b;
    Share e_b = y_b - triple_b.b;
    
    // 第二步：重建d和e的完整值（模拟网络通信）
    Share d = d_a + d_b;  // d = x - a
    Share e = e_a + e_b;  // e = y - b
    
    // 第三步：双方分别计算结果分享
    Share result_a = triple_a.c + d * triple_a.b + e * triple_a.a + d * e;
    Share result_b = triple_b.c + d * triple_b.b + e * triple_b.a;
    
    // 返回重建的乘积
    return result_a + result_b;
}

std::vector<Share> SecretShareDistributor::batchMultiplyWithBeaverTriple(
    const std::vector<Share>& x_shares,
    const std::vector<Share>& y_shares,
    const std::vector<BeaverTriple>& triples,
    bool is_party_a) {
    
    if (x_shares.size() != y_shares.size() || x_shares.size() != triples.size()) {
        throw std::invalid_argument("Input vectors must have the same size");
    }
    
    std::vector<Share> results;
    results.reserve(x_shares.size());
    
    for (size_t i = 0; i < x_shares.size(); ++i) {
        results.push_back(multiplyWithBeaverTriple(x_shares[i], y_shares[i], triples[i], is_party_a));
    }
    
    return results;
}

#ifdef HAVE_APSI
Share SecretShareDistributor::itemToShare(const ItemType& item) {
    // 将APSI Item转换为64位整数
    // 这里假设Item可以直接转换为字节序列
    auto item_bytes = item.get_as<std::vector<std::uint8_t>>();
    
    Share result = 0;
    size_t bytes_to_copy = std::min(sizeof(Share), item_bytes.size());
    
    std::memcpy(&result, item_bytes.data(), bytes_to_copy);
    return result;
}

ItemType SecretShareDistributor::shareToItem(Share share) {
    // 将64位整数转换为APSI Item
    std::vector<std::uint8_t> bytes(sizeof(Share));
    std::memcpy(bytes.data(), &share, sizeof(Share));
    
    return ItemType(bytes);
}

std::vector<Share> SecretShareDistributor::itemsToShares(const std::vector<ItemType>& items) {
    std::vector<Share> shares;
    shares.reserve(items.size());
    
    for (const auto& item : items) {
        shares.push_back(itemToShare(item));
    }
    
    return shares;
}

std::vector<ItemType> SecretShareDistributor::sharesToItems(const std::vector<Share>& shares) {
    std::vector<ItemType> items;
    items.reserve(shares.size());
    
    for (Share share : shares) {
        items.push_back(shareToItem(share));
    }
    
    return items;
}
#endif

Share SecretShareDistributor::reconstructSecret(Share share_a, Share share_b) {
    // 简单的加法秘密分享重建
    return share_a + share_b;
}

void SecretShareDistributor::setTestSeed(uint32_t seed) {
    // 设置OpenSSL的随机种子（仅用于测试）
    RAND_seed(&seed, sizeof(seed));
}

bool SecretShareDistributor::initPRNG(const Seed& seed, EVP_CIPHER_CTX* ctx) {
    return RNGUtils::createCipherContext(seed) != nullptr;
}

bool SecretShareDistributor::generatePRN(EVP_CIPHER_CTX* ctx, uint8_t* output, size_t length) {
    return RNGUtils::generatePseudoRandomBytes(ctx, output, length);
}

Share SecretShareDistributor::generateRandomShare(EVP_CIPHER_CTX* ctx) {
    return RNGUtils::generatePseudoRandomUint64(ctx);
}

} // namespace SecretShare