#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include <openssl/rand.h>
#include <openssl/evp.h>

#ifdef HAVE_APSI
#include <apsi/item.h>
#include <apsi/util/utils.h>
#endif

namespace SecretShare {

// 基础数据类型定义
using Seed = std::vector<uint8_t>;
using Share = uint64_t;

// Beaver Triple结构
struct BeaverTriple {
    Share a;
    Share b;
    Share c;
    
    BeaverTriple() : a(0), b(0), c(0) {}
    BeaverTriple(Share a_val, Share b_val, Share c_val) : a(a_val), b(b_val), c(c_val) {}
};

// APSI Item包装器（如果APSI可用）
#ifdef HAVE_APSI
using ItemType = apsi::Item;
#else
// 如果APSI不可用，使用简单的64位整数作为替代
using ItemType = uint64_t;
#endif

class SecretShareDistributor {
public:
    /**
     * 构造函数
     * @param seed_length 种子长度（字节）
     */
    explicit SecretShareDistributor(size_t seed_length = 32);
    
    /**
     * 析构函数
     */
    ~SecretShareDistributor();
    
    /**
     * 生成并分发种子给A方和B方
     * @param seed_z 给A方的种子
     * @param seed_w 给B方的种子
     * @return 成功返回true，失败返回false
     */
    bool generateAndDistributeSeeds(Seed& seed_z, Seed& seed_w);
    
    /**
     * A方：使用种子生成三元组 [a0, b0, c0]
     * @param seed_z A方的种子
     * @param count 生成三元组的数量
     * @return 三元组向量
     */
    std::vector<BeaverTriple> generateTriplesForPartyA(const Seed& seed_z, size_t count);
    
    /**
     * B方：生成二元组 [a1, b1] 并计算 c1
     * @param seed_w B方的种子
     * @param seed_z A方的种子（用于获取A方三元组）
     * @param count 生成三元组的数量
     * @return 三元组向量
     */
    std::vector<BeaverTriple> generateTriplesForPartyB(const Seed& seed_w, const Seed& seed_z, size_t count);
    
    /**
     * 基于Beaver Triple的算术乘法运算
     * @param x_share 乘数的秘密分享
     * @param y_share 被乘数的秘密分享
     * @param triple Beaver Triple
     * @param is_party_a 是否为A方
     * @return 乘积的秘密分享
     */
    Share multiplyWithBeaverTriple(Share x_share, Share y_share, const BeaverTriple& triple, bool is_party_a);
    
    /**
     * 完整的两方Beaver Triple乘法协议（用于演示）
     * @param x_a A方的x分享
     * @param x_b B方的x分享
     * @param y_a A方的y分享
     * @param y_b B方的y分享
     * @param triple_a A方的三元组
     * @param triple_b B方的三元组
     * @return 乘积的完整值
     */
    Share twoPartyMultiplyWithBeaverTriple(Share x_a, Share x_b, Share y_a, Share y_b, 
                                          const BeaverTriple& triple_a, const BeaverTriple& triple_b);
    
    /**
     * 批量乘法运算
     * @param x_shares 乘数的秘密分享向量
     * @param y_shares 被乘数的秘密分享向量
     * @param triples Beaver Triple向量
     * @param is_party_a 是否为A方
     * @return 乘积的秘密分享向量
     */
    std::vector<Share> batchMultiplyWithBeaverTriple(
        const std::vector<Share>& x_shares,
        const std::vector<Share>& y_shares,
        const std::vector<BeaverTriple>& triples,
        bool is_party_a
    );
    
#ifdef HAVE_APSI
    /**
     * 从APSI Item转换为Share
     * @param item APSI Item
     * @return Share值
     */
    Share itemToShare(const ItemType& item);
    
    /**
     * 从Share转换为APSI Item
     * @param share Share值
     * @return APSI Item
     */
    ItemType shareToItem(Share share);
    
    /**
     * 批量从APSI Item转换为Share
     * @param items APSI Item向量
     * @return Share向量
     */
    std::vector<Share> itemsToShares(const std::vector<ItemType>& items);
    
    /**
     * 批量从Share转换为APSI Item
     * @param shares Share向量
     * @return APSI Item向量
     */
    std::vector<ItemType> sharesToItems(const std::vector<Share>& shares);
#endif
    
    /**
     * 重建秘密（用于验证）
     * @param share_a A方的分享
     * @param share_b B方的分享
     * @return 重建的秘密值
     */
    Share reconstructSecret(Share share_a, Share share_b);
    
    /**
     * 设置随机种子（用于测试）
     * @param seed 种子值
     */
    void setTestSeed(uint32_t seed);

private:
    size_t seed_length_;
    EVP_CIPHER_CTX* cipher_ctx_;
    
    /**
     * 使用种子初始化伪随机数生成器
     * @param seed 种子
     * @param ctx 密码上下文
     * @return 成功返回true
     */
    bool initPRNG(const Seed& seed, EVP_CIPHER_CTX* ctx);
    
    /**
     * 生成伪随机数
     * @param ctx 密码上下文
     * @param output 输出缓冲区
     * @param length 输出长度
     * @return 成功返回true
     */
    bool generatePRN(EVP_CIPHER_CTX* ctx, uint8_t* output, size_t length);
    
    /**
     * 生成随机Share值
     * @param ctx 密码上下文
     * @return 随机Share值
     */
    Share generateRandomShare(EVP_CIPHER_CTX* ctx);
};

} // namespace SecretShare