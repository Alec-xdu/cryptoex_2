#ifndef HASH_TABLES_H
#define HASH_TABLES_H

#include <vector>
#include <unordered_map>
#include <functional>
#include <optional>
#include "psi_item.h"

// 布谷鸟哈希表条目
struct CuckooHashEntry {
    std::vector<uint8_t> oprf_value;
    bool is_occupied;
    
    CuckooHashEntry() : is_occupied(false) {}
    CuckooHashEntry(const std::vector<uint8_t>& value) 
        : oprf_value(value), is_occupied(true) {}
};

// 布谷鸟哈希表
class CuckooHashTable {
private:
    std::vector<CuckooHashEntry> table1_;
    std::vector<CuckooHashEntry> table2_;
    size_t table_size_;
    size_t max_iterations_;
    
    // 哈希函数
    size_t hash1(const std::vector<uint8_t>& data) const;
    size_t hash2(const std::vector<uint8_t>& data) const;
    
    // 插入辅助函数
    bool insertHelper(const std::vector<uint8_t>& value, size_t iterations = 0);
    
public:
    CuckooHashTable(size_t table_size, size_t max_iterations = 8);
    
    // 插入OPRF处理后的值
    bool insert(const std::vector<uint8_t>& oprf_value);
    
    // 查找OPRF处理后的值
    bool contains(const std::vector<uint8_t>& oprf_value) const;
    
    // 批量插入
    bool insertBatch(const std::vector<std::vector<uint8_t>>& oprf_values);
    
    // 获取表大小
    size_t getTableSize() const { return table_size_; }
    
    // 获取负载因子
    double getLoadFactor() const;
    
    // 清空表
    void clear();
    
    // 打印统计信息
    void printStats() const;
};

// 简单哈希表条目
struct SimpleHashEntry {
    std::vector<uint8_t> oprf_value;
    std::vector<uint64_t> polynomial_coeffs; // 多项式系数
    
    SimpleHashEntry() {}
    SimpleHashEntry(const std::vector<uint8_t>& value, 
                   const std::vector<uint64_t>& coeffs)
        : oprf_value(value), polynomial_coeffs(coeffs) {}
};

// 简单哈希表（用于存储多项式系数）
class SimpleHashTable {
private:
    std::unordered_map<std::string, SimpleHashEntry> table_;
    
    // 将字节向量转换为字符串键
    std::string vectorToKey(const std::vector<uint8_t>& data) const;
    
public:
    SimpleHashTable();
    
    // 插入OPRF值和对应的多项式系数
    void insert(const std::vector<uint8_t>& oprf_value, 
               const std::vector<uint64_t>& polynomial_coeffs);
    
    // 查找并获取多项式系数
    std::optional<std::vector<uint64_t>> getPolynomialCoeffs(
        const std::vector<uint8_t>& oprf_value) const;
    
    // 检查是否包含某个OPRF值
    bool contains(const std::vector<uint8_t>& oprf_value) const;
    
    // 批量插入
    void insertBatch(const std::vector<std::vector<uint8_t>>& oprf_values,
                    const std::vector<std::vector<uint64_t>>& polynomial_coeffs);
    
    // 获取表大小
    size_t size() const { return table_.size(); }
    
    // 清空表
    void clear();
    
    // 获取所有OPRF值
    std::vector<std::vector<uint8_t>> getAllOprfValues() const;
    
    // 打印统计信息
    void printStats() const;
};

#endif // HASH_TABLES_H