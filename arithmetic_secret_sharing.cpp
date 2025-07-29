#include "arithmetic_secret_sharing.h"
#include <iostream>
#include <algorithm>
#include <random>

// SecretSharedPolynomial 实现
SecretSharedPolynomial::SecretSharedPolynomial() {
    // 默认构造函数
}

SecretSharedPolynomial::SecretSharedPolynomial(const std::vector<uint64_t>& coeffs) 
    : coefficients_(coeffs) {
}

SecretSharedPolynomial::SecretSharedPolynomial(const SecretSharedPolynomial& other) 
    : coefficients_(other.coefficients_) {
}

SecretSharedPolynomial& SecretSharedPolynomial::operator=(const SecretSharedPolynomial& other) {
    if (this != &other) {
        coefficients_ = other.coefficients_;
    }
    return *this;
}

void SecretSharedPolynomial::setCoefficients(const std::vector<uint64_t>& coeffs) {
    coefficients_ = coeffs;
}

SecretSharedPolynomial SecretSharedPolynomial::operator+(const SecretSharedPolynomial& other) const {
    size_t max_size = std::max(coefficients_.size(), other.coefficients_.size());
    std::vector<uint64_t> result(max_size, 0);
    
    for (size_t i = 0; i < max_size; ++i) {
        uint64_t a = (i < coefficients_.size()) ? coefficients_[i] : 0;
        uint64_t b = (i < other.coefficients_.size()) ? other.coefficients_[i] : 0;
        result[i] = addMod(a, b);
    }
    
    return SecretSharedPolynomial(result);
}

SecretSharedPolynomial SecretSharedPolynomial::operator-(const SecretSharedPolynomial& other) const {
    size_t max_size = std::max(coefficients_.size(), other.coefficients_.size());
    std::vector<uint64_t> result(max_size, 0);
    
    for (size_t i = 0; i < max_size; ++i) {
        uint64_t a = (i < coefficients_.size()) ? coefficients_[i] : 0;
        uint64_t b = (i < other.coefficients_.size()) ? other.coefficients_[i] : 0;
        result[i] = subMod(a, b);
    }
    
    return SecretSharedPolynomial(result);
}

SecretSharedPolynomial SecretSharedPolynomial::operator*(const SecretSharedPolynomial& other) const {
    if (coefficients_.empty() || other.coefficients_.empty()) {
        return SecretSharedPolynomial();
    }
    
    size_t result_size = coefficients_.size() + other.coefficients_.size() - 1;
    std::vector<uint64_t> result(result_size, 0);
    
    for (size_t i = 0; i < coefficients_.size(); ++i) {
        for (size_t j = 0; j < other.coefficients_.size(); ++j) {
            uint64_t prod = mulMod(coefficients_[i], other.coefficients_[j]);
            result[i + j] = addMod(result[i + j], prod);
        }
    }
    
    return SecretSharedPolynomial(result);
}

SecretSharedPolynomial& SecretSharedPolynomial::operator+=(const SecretSharedPolynomial& other) {
    *this = *this + other;
    return *this;
}

SecretSharedPolynomial& SecretSharedPolynomial::operator-=(const SecretSharedPolynomial& other) {
    *this = *this - other;
    return *this;
}

uint64_t SecretSharedPolynomial::evaluate(uint64_t x) const {
    if (coefficients_.empty()) {
        return 0;
    }
    
    // 使用Horner方法计算多项式值
    uint64_t result = coefficients_.back();
    for (int i = static_cast<int>(coefficients_.size()) - 2; i >= 0; --i) {
        result = addMod(mulMod(result, x), coefficients_[i]);
    }
    
    return result;
}

size_t SecretSharedPolynomial::degree() const {
    if (coefficients_.empty()) {
        return 0;
    }
    return coefficients_.size() - 1;
}

void SecretSharedPolynomial::clear() {
    coefficients_.clear();
}

void SecretSharedPolynomial::resize(size_t new_size) {
    coefficients_.resize(new_size, 0);
}

uint64_t SecretSharedPolynomial::addMod(uint64_t a, uint64_t b) const {
    return (a + b) % PRIME_MODULUS;
}

uint64_t SecretSharedPolynomial::subMod(uint64_t a, uint64_t b) const {
    return (a - b + PRIME_MODULUS) % PRIME_MODULUS;
}

uint64_t SecretSharedPolynomial::mulMod(uint64_t a, uint64_t b) const {
    return (a * b) % PRIME_MODULUS;
}

// ArithmeticSecretSharing 实现
ArithmeticSecretSharing::ArithmeticSecretSharing(std::shared_ptr<SecretSharingTEE> tee, bool is_party_a)
    : tee_(tee), is_party_a_(is_party_a), beaver_triplet_index_(0) {
}

ArithmeticSecretSharing::~ArithmeticSecretSharing() {
    // 析构函数
}

bool ArithmeticSecretSharing::initialize(size_t num_triplets) {
    if (!tee_) {
        std::cerr << "TEE not initialized" << std::endl;
        return false;
    }
    
    // 生成Beaver三元组
    tee_->generateAndDistributeShares(num_triplets, party_a_data_, party_b_data_);
    
    // 验证Beaver三元组
    return tee_->verifyBeaverTriplets(party_a_data_.triplets, party_b_data_.pairs, party_b_data_.c1_values);
}

void ArithmeticSecretSharing::setAsPartyA(const SecretSharingTEE::PartyAData& data) {
    party_a_data_ = data;
    is_party_a_ = true;
}

void ArithmeticSecretSharing::setAsPartyB(const SecretSharingTEE::PartyBData& data) {
    party_b_data_ = data;
    is_party_a_ = false;
}

ArithmeticSecretSharing::SharePair ArithmeticSecretSharing::createShare(uint64_t value) {
    // 生成随机份额
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(0, PRIME_MODULUS - 1);
    
    uint64_t share_a = dist(gen);
    uint64_t share_b = subMod(value, share_a);
    
    return SharePair(share_a, share_b);
}

uint64_t ArithmeticSecretSharing::reconstructSecret(const SharePair& shares) {
    return addMod(shares.share_a, shares.share_b);
}

ArithmeticSecretSharing::SharePair ArithmeticSecretSharing::addShares(const SharePair& a, const SharePair& b) {
    return SharePair(
        addMod(a.share_a, b.share_a),
        addMod(a.share_b, b.share_b)
    );
}

ArithmeticSecretSharing::SharePair ArithmeticSecretSharing::subShares(const SharePair& a, const SharePair& b) {
    return SharePair(
        subMod(a.share_a, b.share_a),
        subMod(a.share_b, b.share_b)
    );
}

ArithmeticSecretSharing::SharePair ArithmeticSecretSharing::mulShares(const SharePair& a, const SharePair& b) {
    BeaverTriplet triplet_a;
    BeaverPair pair_b;
    uint64_t c1;
    
    if (!getNextBeaverTriplet(triplet_a, pair_b, c1)) {
        std::cerr << "No more Beaver triplets available" << std::endl;
        return SharePair(0, 0);
    }
    
    // Beaver三元组乘法协议
    // 计算 d = x - a, e = y - b
    uint64_t d_a = subMod(a.share_a, triplet_a.a);
    uint64_t d_b = subMod(a.share_b, pair_b.a);
    uint64_t e_a = subMod(b.share_a, triplet_a.b);
    uint64_t e_b = subMod(b.share_b, pair_b.b);
    
    // 重构 d 和 e
    uint64_t d = addMod(d_a, d_b);
    uint64_t e = addMod(e_a, e_b);
    
    // 计算乘法结果的份额
    uint64_t z_a, z_b;
    
    if (is_party_a_) {
        // A方计算: z_a = c_a + d*b_a + e*a_a + d*e
        z_a = triplet_a.c;
        z_a = addMod(z_a, mulMod(d, triplet_a.b));
        z_a = addMod(z_a, mulMod(e, triplet_a.a));
        z_a = addMod(z_a, mulMod(d, e));
        
        // B方计算: z_b = c_b + d*b_b + e*a_b
        z_b = c1;
        z_b = addMod(z_b, mulMod(d, pair_b.b));
        z_b = addMod(z_b, mulMod(e, pair_b.a));
    } else {
        // 如果当前实例是B方，计算方式相同但使用B方的数据
        z_a = triplet_a.c;
        z_a = addMod(z_a, mulMod(d, triplet_a.b));
        z_a = addMod(z_a, mulMod(e, triplet_a.a));
        z_a = addMod(z_a, mulMod(d, e));
        
        z_b = c1;
        z_b = addMod(z_b, mulMod(d, pair_b.b));
        z_b = addMod(z_b, mulMod(e, pair_b.a));
    }
    
    return SharePair(z_a, z_b);
}

ArithmeticSecretSharing::PolynomialResult ArithmeticSecretSharing::computeMatchingPolynomial(
    const std::vector<uint64_t>& sender_items,
    const SharePair& receiver_query_share) {
    
    PolynomialResult result;
    
    // 计算匹配多项式 M(x) = (x - y1)(x - y2)...(x - yn)
    SecretSharedPolynomial matching_poly = computeSingleMatchingPolynomial(sender_items);
    
    // 在查询点对多项式求值
    result = evaluatePolynomialOnShares(matching_poly, receiver_query_share);
    
    return result;
}

std::vector<ArithmeticSecretSharing::PolynomialResult> ArithmeticSecretSharing::batchPolynomialComputation(
    const std::vector<std::vector<uint64_t>>& bin_bundles,
    const std::vector<SharePair>& receiver_queries) {
    
    std::vector<PolynomialResult> results;
    results.reserve(bin_bundles.size());
    
    for (size_t i = 0; i < bin_bundles.size() && i < receiver_queries.size(); ++i) {
        PolynomialResult result = computeMatchingPolynomial(bin_bundles[i], receiver_queries[i]);
        results.push_back(result);
    }
    
    return results;
}

size_t ArithmeticSecretSharing::getRemainingTriplets() const {
    if (is_party_a_) {
        return party_a_data_.triplets.size() - beaver_triplet_index_;
    } else {
        return party_b_data_.pairs.size() - beaver_triplet_index_;
    }
}

uint64_t ArithmeticSecretSharing::addMod(uint64_t a, uint64_t b) const {
    return (a + b) % PRIME_MODULUS;
}

uint64_t ArithmeticSecretSharing::subMod(uint64_t a, uint64_t b) const {
    return (a - b + PRIME_MODULUS) % PRIME_MODULUS;
}

uint64_t ArithmeticSecretSharing::mulMod(uint64_t a, uint64_t b) const {
    return (a * b) % PRIME_MODULUS;
}

bool ArithmeticSecretSharing::getNextBeaverTriplet(BeaverTriplet& triplet_a, BeaverPair& pair_b, uint64_t& c1) {
    if (beaver_triplet_index_ >= party_a_data_.triplets.size() || 
        beaver_triplet_index_ >= party_b_data_.pairs.size() ||
        beaver_triplet_index_ >= party_b_data_.c1_values.size()) {
        return false;
    }
    
    triplet_a = party_a_data_.triplets[beaver_triplet_index_];
    pair_b = party_b_data_.pairs[beaver_triplet_index_];
    c1 = party_b_data_.c1_values[beaver_triplet_index_];
    
    beaver_triplet_index_++;
    return true;
}

SecretSharedPolynomial ArithmeticSecretSharing::computeSingleMatchingPolynomial(const std::vector<uint64_t>& items) {
    if (items.empty()) {
        return SecretSharedPolynomial();
    }
    
    // 初始化多项式为 1
    SecretSharedPolynomial result({1});
    
    // 逐个乘以 (x - yi)
    for (uint64_t item : items) {
        // 构造 (x - item) = x + (-item)
        std::vector<uint64_t> linear_coeffs = {subMod(0, item), 1};  // -item + x
        SecretSharedPolynomial linear_poly(linear_coeffs);
        
        // 多项式乘法
        result = result * linear_poly;
    }
    
    return result;
}

ArithmeticSecretSharing::PolynomialResult ArithmeticSecretSharing::evaluatePolynomialOnShares(
    const SecretSharedPolynomial& polynomial,
    const SharePair& query_share) {
    
    PolynomialResult result;
    
    if (polynomial.empty()) {
        result.success = false;
        return result;
    }
    
    // 使用Horner方法在秘密共享上计算多项式
    const auto& coeffs = polynomial.getCoefficients();
    
    if (coeffs.empty()) {
        result.success = false;
        return result;
    }
    
    // 从最高次项开始计算
    SharePair poly_result = createShare(coeffs.back());
    
    for (int i = static_cast<int>(coeffs.size()) - 2; i >= 0; --i) {
        // poly_result = poly_result * x + coeffs[i]
        poly_result = mulShares(poly_result, query_share);
        SharePair coeff_share = createShare(coeffs[i]);
        poly_result = addShares(poly_result, coeff_share);
    }
    
    result.evaluation_shares.push_back(poly_result);
    result.success = true;
    
    return result;
}