#include "../include/rng_utils.h"
#include <openssl/rand.h>
#include <openssl/aes.h>
#include <cstring>
#include <stdexcept>

namespace SecretShare {

thread_local uint64_t RNGUtils::counter_ = 0;

std::vector<uint8_t> RNGUtils::generateSecureRandomSeed(size_t length) {
    std::vector<uint8_t> seed(length);
    if (RAND_bytes(seed.data(), static_cast<int>(length)) != 1) {
        throw std::runtime_error("Failed to generate secure random seed");
    }
    return seed;
}

EVP_CIPHER_CTX* RNGUtils::createCipherContext(const std::vector<uint8_t>& seed) {
    if (seed.size() < 16) {
        return nullptr;  // AES需要至少16字节的密钥
    }
    
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return nullptr;
    }
    
    // 使用AES-128-CTR模式
    if (EVP_EncryptInit_ex(ctx, EVP_aes_128_ctr(), nullptr, seed.data(), nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return nullptr;
    }
    
    // 重置计数器
    counter_ = 0;
    
    return ctx;
}

bool RNGUtils::generatePseudoRandomBytes(EVP_CIPHER_CTX* ctx, uint8_t* output, size_t length) {
    if (!ctx || !output || length == 0) {
        return false;
    }
    
    // 创建输入数据（全零，CTR模式会自动处理计数器）
    std::vector<uint8_t> input(length, 0);
    int outlen;
    
    if (EVP_EncryptUpdate(ctx, output, &outlen, input.data(), static_cast<int>(length)) != 1) {
        return false;
    }
    
    counter_ += length;
    return true;
}

uint64_t RNGUtils::generatePseudoRandomUint64(EVP_CIPHER_CTX* ctx) {
    uint64_t result = 0;
    if (!generatePseudoRandomBytes(ctx, reinterpret_cast<uint8_t*>(&result), sizeof(result))) {
        return 0;
    }
    return result;
}

void RNGUtils::cleanupCipherContext(EVP_CIPHER_CTX* ctx) {
    if (ctx) {
        EVP_CIPHER_CTX_free(ctx);
    }
}

} // namespace SecretShare