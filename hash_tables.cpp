#include "hash_tables.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <openssl/sha.h>

// CuckooHashTable实现
CuckooHashTable::CuckooHashTable(size_t table_size, size_t max_iterations)
    : table_size_(table_size), max_iterations_(max_iterations) {
    table1_.resize(table_size_);
    table2_.resize(table_size_);
}

size_t CuckooHashTable::hash1(const std::vector<uint8_t>& data) const {
    // 使用SHA-256的前8字节作为哈希值
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    uint64_t hash_value = 0;
    for (int i = 0; i < 8; ++i) {
        hash_value |= (static_cast<uint64_t>(hash[i]) << (i * 8));
    }
    
    return hash_value % table_size_;
}

size_t CuckooHashTable::hash2(const std::vector<uint8_t>& data) const {
    // 使用SHA-256的后8字节作为哈希值
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    uint64_t hash_value = 0;
    for (int i = 0; i < 8; ++i) {
        hash_value |= (static_cast<uint64_t>(hash[i + 8]) << (i * 8));
    }
    
    return hash_value % table_size_;
}

bool CuckooHashTable::insert(const std::vector<uint8_t>& oprf_value) {
    return insertHelper(oprf_value, 0);
}

bool CuckooHashTable::insertHelper(const std::vector<uint8_t>& value, size_t iterations) {
    if (iterations >= max_iterations_) {
        return false; // 插入失败，可能需要重新哈希
    }
    
    size_t pos1 = hash1(value);
    size_t pos2 = hash2(value);
    
    // 尝试插入到表1
    if (!table1_[pos1].is_occupied) {
        table1_[pos1] = CuckooHashEntry(value);
        return true;
    }
    
    // 尝试插入到表2
    if (!table2_[pos2].is_occupied) {
        table2_[pos2] = CuckooHashEntry(value);
        return true;
    }
    
    // 两个位置都被占用，需要踢出一个元素
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);
    
    if (dis(gen) == 0) {
        // 踢出表1的元素
        auto kicked_value = table1_[pos1].oprf_value;
        table1_[pos1] = CuckooHashEntry(value);
        return insertHelper(kicked_value, iterations + 1);
    } else {
        // 踢出表2的元素
        auto kicked_value = table2_[pos2].oprf_value;
        table2_[pos2] = CuckooHashEntry(value);
        return insertHelper(kicked_value, iterations + 1);
    }
}

bool CuckooHashTable::contains(const std::vector<uint8_t>& oprf_value) const {
    size_t pos1 = hash1(oprf_value);
    size_t pos2 = hash2(oprf_value);
    
    return (table1_[pos1].is_occupied && table1_[pos1].oprf_value == oprf_value) ||
           (table2_[pos2].is_occupied && table2_[pos2].oprf_value == oprf_value);
}

bool CuckooHashTable::insertBatch(const std::vector<std::vector<uint8_t>>& oprf_values) {
    for (const auto& value : oprf_values) {
        if (!insert(value)) {
            std::cerr << "Failed to insert value in cuckoo hash table" << std::endl;
            return false;
        }
    }
    return true;
}

double CuckooHashTable::getLoadFactor() const {
    size_t occupied_count = 0;
    for (const auto& entry : table1_) {
        if (entry.is_occupied) occupied_count++;
    }
    for (const auto& entry : table2_) {
        if (entry.is_occupied) occupied_count++;
    }
    
    return static_cast<double>(occupied_count) / (2 * table_size_);
}

void CuckooHashTable::clear() {
    for (auto& entry : table1_) {
        entry.is_occupied = false;
        entry.oprf_value.clear();
    }
    for (auto& entry : table2_) {
        entry.is_occupied = false;
        entry.oprf_value.clear();
    }
}

void CuckooHashTable::printStats() const {
    std::cout << "Cuckoo Hash Table Statistics:" << std::endl;
    std::cout << "  Table size: " << table_size_ << " x 2" << std::endl;
    std::cout << "  Load factor: " << std::fixed << std::setprecision(3) 
              << getLoadFactor() << std::endl;
    std::cout << "  Max iterations: " << max_iterations_ << std::endl;
}

// SimpleHashTable实现
SimpleHashTable::SimpleHashTable() {
    // 默认构造函数
}

std::string SimpleHashTable::vectorToKey(const std::vector<uint8_t>& data) const {
    std::string key;
    key.reserve(data.size());
    for (uint8_t byte : data) {
        key.push_back(static_cast<char>(byte));
    }
    return key;
}

void SimpleHashTable::insert(const std::vector<uint8_t>& oprf_value,
                           const std::vector<uint64_t>& polynomial_coeffs) {
    std::string key = vectorToKey(oprf_value);
    table_[key] = SimpleHashEntry(oprf_value, polynomial_coeffs);
}

std::optional<std::vector<uint64_t>> SimpleHashTable::getPolynomialCoeffs(
    const std::vector<uint8_t>& oprf_value) const {
    std::string key = vectorToKey(oprf_value);
    auto it = table_.find(key);
    if (it != table_.end()) {
        return it->second.polynomial_coeffs;
    }
    return std::nullopt;
}

bool SimpleHashTable::contains(const std::vector<uint8_t>& oprf_value) const {
    std::string key = vectorToKey(oprf_value);
    return table_.find(key) != table_.end();
}

void SimpleHashTable::insertBatch(const std::vector<std::vector<uint8_t>>& oprf_values,
                                const std::vector<std::vector<uint64_t>>& polynomial_coeffs) {
    if (oprf_values.size() != polynomial_coeffs.size()) {
        std::cerr << "Size mismatch in batch insert" << std::endl;
        return;
    }
    
    for (size_t i = 0; i < oprf_values.size(); ++i) {
        insert(oprf_values[i], polynomial_coeffs[i]);
    }
}

void SimpleHashTable::clear() {
    table_.clear();
}

std::vector<std::vector<uint8_t>> SimpleHashTable::getAllOprfValues() const {
    std::vector<std::vector<uint8_t>> values;
    values.reserve(table_.size());
    
    for (const auto& pair : table_) {
        values.push_back(pair.second.oprf_value);
    }
    
    return values;
}

void SimpleHashTable::printStats() const {
    std::cout << "Simple Hash Table Statistics:" << std::endl;
    std::cout << "  Size: " << table_.size() << " entries" << std::endl;
    std::cout << "  Bucket count: " << table_.bucket_count() << std::endl;
    std::cout << "  Load factor: " << std::fixed << std::setprecision(3) 
              << table_.load_factor() << std::endl;
}