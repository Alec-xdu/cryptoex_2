#include "oprf.h"
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <stdexcept>
#include <cstring>

namespace SecretSharingPSI {

OPRF::OPRF() : initialized_(false) {
}

OPRF::~OPRF() = default;

bool OPRF::generateKey() {
    key_.resize(32);  // 256-bit key
    if (RAND_bytes(key_.data(), key_.size()) != 1) {
        return false;
    }
    initialized_ = true;
    return true;
}

bool OPRF::setKey(const std::vector<uint8_t>& key) {
    if (key.empty()) {
        return false;
    }
    key_ = key;
    initialized_ = true;
    return true;
}

std::vector<uint8_t> OPRF::getKey() const {
    return key_;
}

OPRFItem OPRF::processItem(const Item& item) const {
    if (!initialized_) {
        throw std::runtime_error("OPRF not initialized");
    }
    
    // 获取item的原始数据
    const auto& raw_data = item.getRawData();
    
    // 执行OPRF
    Hash oprf_output = simpleOPRF(raw_data);
    
    // 计算哈希值（用于哈希表的键）
    uint64_t hash_value;
    std::memcpy(&hash_value, oprf_output.data(), sizeof(uint64_t));
    
    return OPRFItem(item, oprf_output, hash_value);
}

std::vector<OPRFItem> OPRF::batchProcess(const std::vector<Item>& items) const {
    std::vector<OPRFItem> results;
    results.reserve(items.size());
    
    for (const auto& item : items) {
        results.push_back(processItem(item));
    }
    
    return results;
}

std::vector<Hash> OPRF::senderBlind(const std::vector<Item>& items) const {
    std::vector<Hash> blinded_hashes;
    blinded_hashes.reserve(items.size());
    
    for (const auto& item : items) {
        // 生成盲化因子
        auto blinding_factor = generateBlindingFactor();
        
        // 应用盲化到item的哈希值
        Hash item_hash = item.getHash();
        std::vector<uint8_t> item_data(item_hash.begin(), item_hash.end());
        auto blinded_data = applyBlinding(item_data, blinding_factor);
        
        // 转换为Hash格式
        Hash blinded_hash;
        std::copy(blinded_data.begin(), 
                 blinded_data.begin() + std::min(blinded_data.size(), blinded_hash.size()), 
                 blinded_hash.begin());
        
        blinded_hashes.push_back(blinded_hash);
    }
    
    return blinded_hashes;
}

std::vector<Hash> OPRF::receiverEvaluate(const std::vector<Hash>& blinded_items) const {
    if (!initialized_) {
        throw std::runtime_error("OPRF not initialized");
    }
    
    std::vector<Hash> outputs;
    outputs.reserve(blinded_items.size());
    
    for (const auto& blinded_item : blinded_items) {
        // 将Hash转换为vector<uint8_t>
        std::vector<uint8_t> blinded_data(blinded_item.begin(), blinded_item.end());
        
        // 执行OPRF
        Hash output = simpleOPRF(blinded_data);
        outputs.push_back(output);
    }
    
    return outputs;
}

std::vector<OPRFItem> OPRF::senderUnblind(const std::vector<Hash>& oprf_outputs,
                                         const std::vector<Item>& original_items) const {
    if (oprf_outputs.size() != original_items.size()) {
        throw std::invalid_argument("Mismatched sizes between OPRF outputs and original items");
    }
    
    std::vector<OPRFItem> results;
    results.reserve(original_items.size());
    
    for (size_t i = 0; i < original_items.size(); ++i) {
        // 在简化实现中，我们直接使用OPRF输出
        // 在完整实现中，这里需要去除盲化
        uint64_t hash_value;
        std::memcpy(&hash_value, oprf_outputs[i].data(), sizeof(uint64_t));
        
        results.emplace_back(original_items[i], oprf_outputs[i], hash_value);
    }
    
    return results;
}

Hash OPRF::simpleOPRF(const std::vector<uint8_t>& input) const {
    if (!initialized_) {
        throw std::runtime_error("OPRF not initialized");
    }
    
    return hmacHash(input, key_);
}

bool OPRF::isInitialized() const {
    return initialized_;
}

Hash OPRF::hmacHash(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key) const {
    Hash result;
    unsigned int result_len;
    
    if (!HMAC(EVP_sha256(), key.data(), key.size(), 
              data.data(), data.size(), 
              result.data(), &result_len)) {
        throw std::runtime_error("HMAC computation failed");
    }
    
    if (result_len != HASH_SIZE) {
        throw std::runtime_error("Unexpected HMAC output size");
    }
    
    return result;
}

std::vector<uint8_t> OPRF::generateBlindingFactor() const {
    std::vector<uint8_t> factor(32);  // 256-bit blinding factor
    if (RAND_bytes(factor.data(), factor.size()) != 1) {
        throw std::runtime_error("Failed to generate blinding factor");
    }
    return factor;
}

std::vector<uint8_t> OPRF::applyBlinding(const std::vector<uint8_t>& data,
                                        const std::vector<uint8_t>& blinding_factor) const {
    std::vector<uint8_t> result(data.size());
    
    for (size_t i = 0; i < data.size(); ++i) {
        result[i] = data[i] ^ blinding_factor[i % blinding_factor.size()];
    }
    
    return result;
}

std::vector<uint8_t> OPRF::removeBlinding(const std::vector<uint8_t>& blinded_data,
                                         const std::vector<uint8_t>& blinding_factor) const {
    // XOR是自逆运算，所以去盲化和盲化使用相同的操作
    return applyBlinding(blinded_data, blinding_factor);
}

} // namespace SecretSharingPSI