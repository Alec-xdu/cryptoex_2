#include "cuckoo_hash.h"
#include <algorithm>
#include <random>
#include <stdexcept>

namespace SecretSharingPSI {

CuckooHashTable::CuckooHashTable(size_t capacity, size_t max_iterations)
    : capacity_(capacity), size_(0), max_iterations_(max_iterations),
      rehash_count_(0), kick_count_(0), max_kick_sequence_(0) {
    if (capacity_ == 0) {
        capacity_ = 1024;
    }
    
    for (size_t i = 0; i < NUM_TABLES; ++i) {
        tables_[i].resize(capacity_);
    }
    
    initializeHashSeeds();
}

CuckooHashTable::~CuckooHashTable() = default;

bool CuckooHashTable::insert(uint64_t key, uint64_t value) {
    // 首先尝试简单插入
    if (trySimpleInsert(key, value)) {
        size_++;
        return true;
    }
    
    // 如果简单插入失败，使用布谷鸟踢出算法
    for (size_t table_id = 0; table_id < NUM_TABLES; ++table_id) {
        if (cuckooKick(key, value, table_id)) {
            size_++;
            return true;
        }
    }
    
    // 如果踢出算法也失败，尝试重建哈希表
    if (rebuild()) {
        return insert(key, value);  // 递归重试
    }
    
    return false;
}

bool CuckooHashTable::find(uint64_t key, uint64_t& value) const {
    for (size_t table_id = 0; table_id < NUM_TABLES; ++table_id) {
        size_t pos = hash(key, table_id);
        if (tables_[table_id][pos].is_occupied && tables_[table_id][pos].key == key) {
            value = tables_[table_id][pos].value;
            return true;
        }
    }
    return false;
}

bool CuckooHashTable::contains(uint64_t key) const {
    uint64_t dummy;
    return find(key, dummy);
}

bool CuckooHashTable::remove(uint64_t key) {
    for (size_t table_id = 0; table_id < NUM_TABLES; ++table_id) {
        size_t pos = hash(key, table_id);
        if (tables_[table_id][pos].is_occupied && tables_[table_id][pos].key == key) {
            tables_[table_id][pos].is_occupied = false;
            size_--;
            return true;
        }
    }
    return false;
}

void CuckooHashTable::clear() {
    for (auto& table : tables_) {
        for (auto& entry : table) {
            entry.is_occupied = false;
        }
    }
    size_ = 0;
    rehash_count_ = 0;
    kick_count_ = 0;
    max_kick_sequence_ = 0;
}

size_t CuckooHashTable::size() const {
    return size_;
}

size_t CuckooHashTable::capacity() const {
    return capacity_;
}

double CuckooHashTable::getCurrentLoadFactor() const {
    return static_cast<double>(size_) / (capacity_ * NUM_TABLES);
}

bool CuckooHashTable::empty() const {
    return size_ == 0;
}

size_t CuckooHashTable::batchInsert(const std::vector<OPRFItem>& oprf_items) {
    size_t success_count = 0;
    for (const auto& item : oprf_items) {
        if (insert(item.hash_value, item.original_item.getValue())) {
            success_count++;
        }
    }
    return success_count;
}

std::vector<std::pair<uint64_t, uint64_t>> CuckooHashTable::batchFind(const std::vector<uint64_t>& keys) const {
    std::vector<std::pair<uint64_t, uint64_t>> results;
    results.reserve(keys.size());
    
    for (uint64_t key : keys) {
        uint64_t value;
        if (find(key, value)) {
            results.emplace_back(key, value);
        }
    }
    
    return results;
}

CuckooHashTable::Statistics CuckooHashTable::getStatistics() const {
    Statistics stats;
    stats.element_count = size_;
    stats.capacity = capacity_ * NUM_TABLES;
    stats.load_factor = getCurrentLoadFactor();
    stats.rehash_count = rehash_count_;
    stats.kick_count = kick_count_;
    stats.max_kick_sequence = max_kick_sequence_;
    stats.success_rate = size_ > 0 ? 1.0 - static_cast<double>(rehash_count_) / size_ : 1.0;
    
    return stats;
}

bool CuckooHashTable::rebuild(size_t new_capacity) {
    if (new_capacity == 0) {
        new_capacity = std::max(capacity_ * 2, size_ * 3);  // 确保有足够空间
    }
    
    // 保存旧数据
    std::vector<std::pair<uint64_t, uint64_t>> old_entries;
    for (const auto& table : tables_) {
        for (const auto& entry : table) {
            if (entry.is_occupied) {
                old_entries.emplace_back(entry.key, entry.value);
            }
        }
    }
    
    // 重建哈希表
    capacity_ = new_capacity;
    size_ = 0;
    rehash_count_++;
    
    for (auto& table : tables_) {
        table.clear();
        table.resize(capacity_);
    }
    
    generateNewHashSeeds();
    
    // 重新插入所有数据
    for (const auto& entry : old_entries) {
        if (!insert(entry.first, entry.second)) {
            return false;  // 重建失败
        }
    }
    
    return true;
}

size_t CuckooHashTable::hash(uint64_t key, size_t table_id) const {
    if (table_id >= NUM_TABLES) {
        return 0;
    }
    
    // 使用不同的哈希种子为每个表生成不同的哈希值
    uint64_t hash_value = key ^ hash_seeds_[table_id];
    hash_value ^= hash_value >> 33;
    hash_value *= 0xff51afd7ed558ccdULL;
    hash_value ^= hash_value >> 33;
    hash_value *= 0xc4ceb9fe1a85ec53ULL;
    hash_value ^= hash_value >> 33;
    
    return hash_value % capacity_;
}

void CuckooHashTable::initializeHashSeeds() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    
    for (size_t i = 0; i < NUM_TABLES; ++i) {
        hash_seeds_[i] = gen();
    }
}

bool CuckooHashTable::cuckooKick(uint64_t key, uint64_t value, size_t start_table) {
    uint64_t current_key = key;
    uint64_t current_value = value;
    size_t current_table = start_table;
    size_t kick_sequence = 0;
    
    for (size_t iteration = 0; iteration < max_iterations_; ++iteration) {
        size_t pos = hash(current_key, current_table);
        
        if (!tables_[current_table][pos].is_occupied) {
            // 找到空位，直接插入
            tables_[current_table][pos] = HashTableEntry(current_key, current_value);
            max_kick_sequence_ = std::max(max_kick_sequence_, kick_sequence);
            return true;
        }
        
        // 踢出现有元素
        std::swap(current_key, tables_[current_table][pos].key);
        std::swap(current_value, tables_[current_table][pos].value);
        
        // 切换到另一个表
        current_table = (current_table + 1) % NUM_TABLES;
        kick_count_++;
        kick_sequence++;
    }
    
    return false;  // 踢出失败
}

bool CuckooHashTable::trySimpleInsert(uint64_t key, uint64_t value) {
    for (size_t table_id = 0; table_id < NUM_TABLES; ++table_id) {
        size_t pos = hash(key, table_id);
        
        if (tables_[table_id][pos].is_occupied && tables_[table_id][pos].key == key) {
            // 键已存在，更新值
            tables_[table_id][pos].value = value;
            return true;  // 不增加size_，因为是更新
        }
        
        if (!tables_[table_id][pos].is_occupied) {
            // 找到空位，直接插入
            tables_[table_id][pos] = HashTableEntry(key, value);
            return true;
        }
    }
    
    return false;
}

size_t CuckooHashTable::findInTable(uint64_t key, size_t table_id) const {
    if (table_id >= NUM_TABLES) {
        return capacity_;
    }
    
    size_t pos = hash(key, table_id);
    if (tables_[table_id][pos].is_occupied && tables_[table_id][pos].key == key) {
        return pos;
    }
    
    return capacity_;  // 未找到
}

bool CuckooHashTable::needsRebuild() const {
    return getCurrentLoadFactor() > 0.9;  // 当负载因子超过90%时重建
}

void CuckooHashTable::generateNewHashSeeds() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    
    for (size_t i = 0; i < NUM_TABLES; ++i) {
        hash_seeds_[i] = gen();
    }
}

} // namespace SecretSharingPSI