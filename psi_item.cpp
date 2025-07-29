#include "psi_item.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <openssl/rand.h>

// PSIItem实现
PSIItem::PSIItem() {
    // 默认构造函数
}

PSIItem::PSIItem(const std::string& str_data) {
    setData(str_data);
}

PSIItem::PSIItem(const std::vector<uint8_t>& raw_data) {
    setData(raw_data);
}

PSIItem::PSIItem(const PSIItem& other) 
    : data_(other.data_), hash_(other.hash_) {
}

PSIItem& PSIItem::operator=(const PSIItem& other) {
    if (this != &other) {
        data_ = other.data_;
        hash_ = other.hash_;
    }
    return *this;
}

void PSIItem::setData(const std::string& str_data) {
    data_.assign(str_data.begin(), str_data.end());
    computeHash();
}

void PSIItem::setData(const std::vector<uint8_t>& raw_data) {
    data_ = raw_data;
    computeHash();
}

void PSIItem::computeHash() {
    hash_.resize(SHA256_DIGEST_LENGTH);
    SHA256(data_.data(), data_.size(), hash_.data());
}

bool PSIItem::operator==(const PSIItem& other) const {
    return hash_ == other.hash_;
}

bool PSIItem::operator!=(const PSIItem& other) const {
    return !(*this == other);
}

bool PSIItem::operator<(const PSIItem& other) const {
    return hash_ < other.hash_;
}

std::string PSIItem::toString() const {
    std::ostringstream oss;
    oss << "Data: ";
    for (size_t i = 0; i < std::min(data_.size(), size_t(16)); ++i) {
        if (data_[i] >= 32 && data_[i] <= 126) {
            oss << static_cast<char>(data_[i]);
        } else {
            oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data_[i]);
        }
    }
    if (data_.size() > 16) {
        oss << "...";
    }
    
    oss << ", Hash: ";
    for (size_t i = 0; i < std::min(hash_.size(), size_t(8)); ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash_[i]);
    }
    oss << "...";
    
    return oss.str();
}

// OPRFProcessor实现
OPRFProcessor::OPRFProcessor() {
    generateKey();
}

OPRFProcessor::OPRFProcessor(const std::vector<uint8_t>& key) {
    setKey(key);
}

void OPRFProcessor::generateKey() {
    key_.resize(KEY_SIZE);
    if (RAND_bytes(key_.data(), KEY_SIZE) != 1) {
        std::cerr << "Failed to generate OPRF key" << std::endl;
        exit(1);
    }
}

void OPRFProcessor::setKey(const std::vector<uint8_t>& key) {
    if (key.size() != KEY_SIZE) {
        std::cerr << "Invalid OPRF key size. Expected " << KEY_SIZE << " bytes, got " << key.size() << std::endl;
        exit(1);
    }
    key_ = key;
}

std::vector<uint8_t> OPRFProcessor::processItem(const PSIItem& item) const {
    return hmacSha256(item.getHash());
}

std::vector<std::vector<uint8_t>> OPRFProcessor::processItems(const std::vector<PSIItem>& items) const {
    std::vector<std::vector<uint8_t>> results;
    results.reserve(items.size());
    
    for (const auto& item : items) {
        results.push_back(processItem(item));
    }
    
    return results;
}

std::vector<uint8_t> OPRFProcessor::hmacSha256(const std::vector<uint8_t>& data) const {
    std::vector<uint8_t> result(SHA256_DIGEST_LENGTH);
    unsigned int result_len;
    
    unsigned char* hmac_result = HMAC(EVP_sha256(), 
                                     key_.data(), key_.size(),
                                     data.data(), data.size(),
                                     result.data(), &result_len);
    
    if (hmac_result == nullptr || result_len != SHA256_DIGEST_LENGTH) {
        std::cerr << "HMAC computation failed" << std::endl;
        exit(1);
    }
    
    return result;
}