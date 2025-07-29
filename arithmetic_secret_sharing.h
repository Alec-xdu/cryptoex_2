#ifndef ARITHMETIC_SECRET_SHARING_H
#define ARITHMETIC_SECRET_SHARING_H

#include "secret_sharing_tee.h"
#include <vector>
#include <cstdint>
#include <memory>

// 算数秘密共享多项式类
class SecretSharedPolynomial {
private:
    std::vector<uint64_t> coefficients_;
    static const uint64_t PRIME_MODULUS = SecretSharingTEE::getPrimeModulus();
    
public:
    SecretSharedPolynomial();
    SecretSharedPolynomial(const std::vector<uint64_t>& coeffs);
    SecretSharedPolynomial(const SecretSharedPolynomial& other);
    SecretSharedPolynomial& operator=(const SecretSharedPolynomial& other);
    
    // 获取系数
    const std::vector<uint64_t>& getCoefficients() const { return coefficients_; }
    void setCoefficients(const std::vector<uint64_t>& coeffs);
    
    // 多项式运算
    SecretSharedPolynomial operator+(const SecretSharedPolynomial& other) const;
    SecretSharedPolynomial operator-(const SecretSharedPolynomial& other) const;
    SecretSharedPolynomial operator*(const SecretSharedPolynomial& other) const;
    SecretSharedPolynomial& operator+=(const SecretSharedPolynomial& other);
    SecretSharedPolynomial& operator-=(const SecretSharedPolynomial& other);
    
    // 在某点求值
    uint64_t evaluate(uint64_t x) const;
    
    // 获取度数
    size_t degree() const;
    
    // 清空
    void clear();
    bool empty() const { return coefficients_.empty(); }
    
    // 调整大小
    void resize(size_t new_size);
    
private:
    // 模运算辅助函数
    uint64_t addMod(uint64_t a, uint64_t b) const;
    uint64_t subMod(uint64_t a, uint64_t b) const;
    uint64_t mulMod(uint64_t a, uint64_t b) const;
};

// 算数秘密共享运算器
class ArithmeticSecretSharing {
private:
    std::shared_ptr<SecretSharingTEE> tee_;
    SecretSharingTEE::PartyAData party_a_data_;
    SecretSharingTEE::PartyBData party_b_data_;
    bool is_party_a_;
    size_t beaver_triplet_index_;
    
    static const uint64_t PRIME_MODULUS = SecretSharingTEE::getPrimeModulus();
    
public:
    ArithmeticSecretSharing(std::shared_ptr<SecretSharingTEE> tee, bool is_party_a = true);
    ~ArithmeticSecretSharing();
    
    // 初始化：生成或接收Beaver三元组
    bool initialize(size_t num_triplets);
    
    // 设置作为A方或B方
    void setAsPartyA(const SecretSharingTEE::PartyAData& data);
    void setAsPartyB(const SecretSharingTEE::PartyBData& data);
    
    // 秘密共享运算
    struct SharePair {
        uint64_t share_a;  // A方的份额
        uint64_t share_b;  // B方的份额
        
        SharePair() : share_a(0), share_b(0) {}
        SharePair(uint64_t a, uint64_t b) : share_a(a), share_b(b) {}
    };
    
    // 创建秘密共享
    SharePair createShare(uint64_t value);
    
    // 重构秘密值
    uint64_t reconstructSecret(const SharePair& shares);
    
    // 秘密共享加法（本地运算）
    SharePair addShares(const SharePair& a, const SharePair& b);
    
    // 秘密共享减法（本地运算）
    SharePair subShares(const SharePair& a, const SharePair& b);
    
    // 秘密共享乘法（需要Beaver三元组）
    SharePair mulShares(const SharePair& a, const SharePair& b);
    
    // 多项式匹配运算
    struct PolynomialResult {
        std::vector<SharePair> evaluation_shares;  // 多项式在各点的求值份额
        bool success;
        
        PolynomialResult() : success(false) {}
    };
    
    // 计算匹配多项式：M(x) = (x - y1)(x - y2)...(x - yn)
    PolynomialResult computeMatchingPolynomial(
        const std::vector<uint64_t>& sender_items,  // sender的item哈希值
        const SharePair& receiver_query_share       // receiver的查询项份额
    );
    
    // 批量多项式运算
    std::vector<PolynomialResult> batchPolynomialComputation(
        const std::vector<std::vector<uint64_t>>& bin_bundles,  // 分组的sender items
        const std::vector<SharePair>& receiver_queries          // receiver的查询项份额
    );
    
    // 获取剩余Beaver三元组数量
    size_t getRemainingTriplets() const;
    
    // 重置Beaver三元组索引
    void resetTripletIndex() { beaver_triplet_index_ = 0; }
    
private:
    // 模运算辅助函数
    uint64_t addMod(uint64_t a, uint64_t b) const;
    uint64_t subMod(uint64_t a, uint64_t b) const;
    uint64_t mulMod(uint64_t a, uint64_t b) const;
    
    // 获取下一个Beaver三元组
    bool getNextBeaverTriplet(BeaverTriplet& triplet_a, BeaverPair& pair_b, uint64_t& c1);
    
    // 计算单个匹配多项式
    SecretSharedPolynomial computeSingleMatchingPolynomial(const std::vector<uint64_t>& items);
    
    // 对多项式进行秘密共享求值
    PolynomialResult evaluatePolynomialOnShares(
        const SecretSharedPolynomial& polynomial,
        const SharePair& query_share
    );
};

#endif // ARITHMETIC_SECRET_SHARING_H