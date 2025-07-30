#include "hash_table.h"
#include <algorithm>
#include <stdexcept>

namespace SecretSharingPSI {

HashTable::HashTable(size_t capacity, double load_factor) 
    : capacity_(capacity), size_(0), max_load_factor_(load_factor), collision_count_(0) {
    if (capacity_ == 0) {
        capacity_ = 1024;
    }
    table_.resize(capacity_);
}

HashTable::~HashTable() = default;

bool HashTable::insert(uint64_t key, uint64_t value) {
    if (needsResize()) {
        resize(capacity_ * 2);
    }
    
    size_t pos = findInsertPosition(key);
    if (pos < capacity_) {
        if (table_[pos].is_occupied && table_[pos].key == key) {
            // 键已存在，更新值
            table_[pos].value = value;
            return true;
        } else if (!table_[pos].is_occupied) {
            // 插入新键值对
            table_[pos] = HashTableEntry(key, value);
            size_++;
            return true;
        }
    }
    
    return false;
}

bool HashTable::find(uint64_t key, uint64_t& value) const {
    size_t pos = findPosition(key);
    if (pos < capacity_ && table_[pos].is_occupied && table_[pos].key == key) {
        value = table_[pos].value;
        return true;
    }
    return false;
}

bool HashTable::contains(uint64_t key) const {
    uint64_t dummy;
    return find(key, dummy);
}

bool HashTable::remove(uint64_t key) {
    size_t pos = findPosition(key);
    if (pos < capacity_ && table_[pos].is_occupied && table_[pos].key == key) {
        table_[pos].is_occupied = false;
        size_--;
        return true;
    }
    return false;
}

void HashTable::clear() {
    for (auto& entry : table_) {
        entry.is_occupied = false;
    }
    size_ = 0;
    collision_count_ = 0;
}

size_t HashTable::size() const {
    return size_;
}

size_t HashTable::capacity() const {
    return capacity_;
}

double HashTable::getCurrentLoadFactor() const {
    return static_cast<double>(size_) / capacity_;
}

bool HashTable::empty() const {
    return size_ == 0;
}

size_t HashTable::batchInsert(const std::vector<OPRFItem>& oprf_items) {
    size_t success_count = 0;
    for (const auto& item : oprf_items) {
        if (insert(item.hash_value, item.original_item.getValue())) {
            success_count++;
        }
    }
    return success_count;
}

std::vector<std::pair<uint64_t, uint64_t>> HashTable::batchFind(const std::vector<uint64_t>& keys) const {
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

HashTable::Statistics HashTable::getStatistics() const {
    Statistics stats;
    stats.element_count = size_;
    stats.capacity = capacity_;
    stats.load_factor = getCurrentLoadFactor();
    stats.collision_count = collision_count_;
    
    // 计算探测距离统计
    size_t total_probe_distance = 0;
    size_t max_probe_distance = 0;
    size_t occupied_count = 0;
    
    for (size_t i = 0; i < capacity_; ++i) {
        if (table_[i].is_occupied) {
            size_t ideal_pos = hash(table_[i].key);
            size_t probe_distance = (i >= ideal_pos) ? (i - ideal_pos) : (capacity_ - ideal_pos + i);
            total_probe_distance += probe_distance;
            max_probe_distance = std::max(max_probe_distance, probe_distance);
            occupied_count++;
        }
    }
    
    stats.max_probe_distance = max_probe_distance;
    stats.average_probe_distance = occupied_count > 0 ? 
        static_cast<double>(total_probe_distance) / occupied_count : 0.0;
    
    return stats;
}

void HashTable::resize(size_t new_capacity) {
    if (new_capacity <= capacity_) {
        return;
    }
    
    std::vector<HashTableEntry> old_table = std::move(table_);
    
    capacity_ = new_capacity;
    table_.clear();
    table_.resize(capacity_);
    size_ = 0;
    collision_count_ = 0;
    
    // 重新插入所有元素
    for (const auto& entry : old_table) {
        if (entry.is_occupied) {
            insert(entry.key, entry.value);
        }
    }
}

size_t HashTable::hash(uint64_t key) const {
    // 使用简单的乘法哈希
    const uint64_t A = 11400714819323198485ULL;  // 大素数
    return (A * key) % capacity_;
}

size_t HashTable::hash2(uint64_t key) const {
    // 第二个哈希函数，用于双重哈希
    const uint64_t A = 14029467366897019727ULL;  // 另一个大素数
    size_t h = (A * key) % capacity_;
    return h == 0 ? 1 : h;  // 确保不为0
}

size_t HashTable::findPosition(uint64_t key) const {
    size_t pos = hash(key);
    size_t step = hash2(key);
    size_t attempts = 0;
    
    while (attempts < capacity_) {
        if (!table_[pos].is_occupied || table_[pos].key == key) {
            return pos;
        }
        collision_count_++;
        pos = (pos + step) % capacity_;
        attempts++;
    }
    
    return capacity_;  // 表示未找到
}

size_t HashTable::findInsertPosition(uint64_t key) const {
    return findPosition(key);
}

bool HashTable::needsResize() const {
    return getCurrentLoadFactor() > max_load_factor_;
}

} // namespace SecretSharingPSI