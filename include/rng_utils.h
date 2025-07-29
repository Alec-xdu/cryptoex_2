#pragma once

#include <vector>
#include <cstdint>
#include <openssl/evp.h>

namespace SecretShare {

/**
 * RNG工具类，提供基于OpenSSL的伪随机数生成功能
 */
class RNGUtils {
public:
    /**
     * 生成安全随机种子
     * @param length 种子长度（字节）
     * @return 随机种子
     */
    static std::vector<uint8_t> generateSecureRandomSeed(size_t length);
    
    /**
     * 创建并初始化AES-CTR模式的密码上下文
     * @param seed 种子
     * @return 密码上下文指针，失败返回nullptr
     */
    static EVP_CIPHER_CTX* createCipherContext(const std::vector<uint8_t>& seed);
    
    /**
     * 生成伪随机字节序列
     * @param ctx 密码上下文
     * @param output 输出缓冲区
     * @param length 输出长度
     * @return 成功返回true
     */
    static bool generatePseudoRandomBytes(EVP_CIPHER_CTX* ctx, uint8_t* output, size_t length);
    
    /**
     * 生成伪随机64位整数
     * @param ctx 密码上下文
     * @return 伪随机64位整数
     */
    static uint64_t generatePseudoRandomUint64(EVP_CIPHER_CTX* ctx);
    
    /**
     * 清理密码上下文
     * @param ctx 密码上下文
     */
    static void cleanupCipherContext(EVP_CIPHER_CTX* ctx);

private:
    static thread_local uint64_t counter_;  // 用于CTR模式的计数器
};

} // namespace SecretShare