#include "item.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <cstring>

namespace SecretSharingPSI {

Item::Item() : hash_computed_(false) {
    cached_hash_.fill(0);
}

Item::Item(const std::string& data) : hash_computed_(false) {
    data_.assign(data.begin(), data.end());
    cached_hash_.fill(0);
}

Item::Item(const uint8_t* data, size_t size) : hash_computed_(false) {
    if (data && size > 0) {
        data_.assign(data, data + size);
    }
    cached_hash_.fill(0);
}

Item::Item(uint64_t value) : hash_computed_(false) {
    data_.resize(sizeof(uint64_t));
    std::memcpy(data_.data(), &value, sizeof(uint64_t));
    cached_hash_.fill(0);
}

Item::Item(const Item& other) 
    : data_(other.data_), cached_hash_(other.cached_hash_), 
      hash_computed_(other.hash_computed_) {
}

Item& Item::operator=(const Item& other) {
    if (this != &other) {
        data_ = other.data_;
        cached_hash_ = other.cached_hash_;
        hash_computed_ = other.hash_computed_;
    }
    return *this;
}

Item::Item(Item&& other) noexcept 
    : data_(std::move(other.data_)), cached_hash_(std::move(other.cached_hash_)),
      hash_computed_(other.hash_computed_) {
    other.hash_computed_ = false;
}

Item& Item::operator=(Item&& other) noexcept {
    if (this != &other) {
        data_ = std::move(other.data_);
        cached_hash_ = std::move(other.cached_hash_);
        hash_computed_ = other.hash_computed_;
        other.hash_computed_ = false;
    }
    return *this;
}

Item::~Item() = default;

Hash Item::getHash() const {
    if (!hash_computed_) {
        computeHash();
    }
    return cached_hash_;
}

uint64_t Item::getValue() const {
    if (data_.size() >= sizeof(uint64_t)) {
        uint64_t value;
        std::memcpy(&value, data_.data(), sizeof(uint64_t));
        return value;
    } else {
        // 对于较小的数据，计算哈希并取前8字节
        Hash hash = getHash();
        uint64_t value;
        std::memcpy(&value, hash.data(), sizeof(uint64_t));
        return value;
    }
}

const std::vector<uint8_t>& Item::getRawData() const {
    return data_;
}

size_t Item::size() const {
    return data_.size();
}

bool Item::empty() const {
    return data_.empty();
}

void Item::clear() {
    data_.clear();
    cached_hash_.fill(0);
    hash_computed_ = false;
}

std::string Item::toString() const {
    std::ostringstream oss;
    for (uint8_t byte : data_) {
        if (std::isprint(byte)) {
            oss << static_cast<char>(byte);
        } else {
            oss << "\\x" << std::hex << std::setw(2) << std::setfill('0') 
                << static_cast<int>(byte);
        }
    }
    return oss.str();
}

bool Item::operator==(const Item& other) const {
    return data_ == other.data_;
}

bool Item::operator!=(const Item& other) const {
    return !(*this == other);
}

bool Item::operator<(const Item& other) const {
    return data_ < other.data_;
}

std::vector<Hash> Item::batchHash(const std::vector<Item>& items) {
    std::vector<Hash> hashes;
    hashes.reserve(items.size());
    
    for (const auto& item : items) {
        hashes.push_back(item.getHash());
    }
    
    return hashes;
}

std::vector<Item> Item::loadFromFile(const std::string& filename) {
    std::vector<Item> items;
    std::ifstream file(filename);
    
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) {
            items.emplace_back(line);
        }
    }
    
    return items;
}

bool Item::saveToFile(const std::vector<Item>& items, const std::string& filename) {
    std::ofstream file(filename);
    
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& item : items) {
        // 将字节数据转换为十六进制字符串保存
        for (uint8_t byte : item.data_) {
            file << std::hex << std::setw(2) << std::setfill('0') 
                 << static_cast<int>(byte);
        }
        file << std::endl;
    }
    
    return true;
}

void Item::computeHash() const {
    if (data_.empty()) {
        cached_hash_.fill(0);
        hash_computed_ = true;
        return;
    }
    
    // 使用SHA-256计算哈希
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Failed to create hash context");
    }
    
    if (EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to initialize hash");
    }
    
    if (EVP_DigestUpdate(ctx, data_.data(), data_.size()) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to update hash");
    }
    
    unsigned int hash_len;
    if (EVP_DigestFinal_ex(ctx, cached_hash_.data(), &hash_len) != 1) {
        EVP_MD_CTX_free(ctx);
        throw std::runtime_error("Failed to finalize hash");
    }
    
    EVP_MD_CTX_free(ctx);
    
    if (hash_len != HASH_SIZE) {
        throw std::runtime_error("Unexpected hash size");
    }
    
    hash_computed_ = true;
}

} // namespace SecretSharingPSI