#pragma once

#include "common.h"
#include "item.h"
#include <vector>
#include <array>

namespace SecretSharingPSI {

/**
 * 布谷鸟哈希表实现
 * 用于PSI协议中发送方的数据存储，提供更好的查询性能
 */
class CuckooHashTable {
public:
    /**
     * 构造函数
     * @param capacity 哈希表容量
     * @param max_iterations 最大重哈希迭代次数
     */
    explicit CuckooHashTable(size_t capacity = 1024, size_t max_iterations = 8);

    /**
     * 析构函数
     */
    ~CuckooHashTable();

    /**
     * 插入键值对
     * @param key 键
     * @param value 值
     * @return 是否成功插入
     */
    bool insert(uint64_t key, uint64_t value);

    /**
     * 查找键对应的值
     * @param key 键
     * @param value 输出参数，存储找到的值
     * @return 是否找到
     */
    bool find(uint64_t key, uint64_t& value) const;

    /**
     * 检查键是否存在
     * @param key 键
     * @return 是否存在
     */
    bool contains(uint64_t key) const;

    /**
     * 删除键值对
     * @param key 键
     * @return 是否成功删除
     */
    bool remove(uint64_t key);

    /**
     * 清空哈希表
     */
    void clear();

    /**
     * 获取当前元素数量
     * @return 元素数量
     */
    size_t size() const;

    /**
     * 获取哈希表容量
     * @return 容量
     */
    size_t capacity() const;

    /**
     * 获取当前负载因子
     * @return 负载因子
     */
    double getCurrentLoadFactor() const;

    /**
     * 检查哈希表是否为空
     * @return 是否为空
     */
    bool empty() const;

    /**
     * 批量插入OPRF处理后的items
     * @param oprf_items OPRF处理后的items
     * @return 成功插入的数量
     */
    size_t batchInsert(const std::vector<OPRFItem>& oprf_items);

    /**
     * 批量查询
     * @param keys 要查询的键列表
     * @return 查询结果，包含找到的键值对
     */
    std::vector<std::pair<uint64_t, uint64_t>> batchFind(const std::vector<uint64_t>& keys) const;

    /**
     * 获取统计信息
     */
    struct Statistics {
        size_t element_count;
        size_t capacity;
        double load_factor;
        size_t rehash_count;
        size_t kick_count;
        size_t max_kick_sequence;
        double success_rate;
    };
    Statistics getStatistics() const;

    /**
     * 重建哈希表
     * @param new_capacity 新容量（可选）
     * @return 是否成功重建
     */
    bool rebuild(size_t new_capacity = 0);

private:
    static constexpr size_t NUM_TABLES = 2;  // 使用两个哈希表
    
    std::array<std::vector<HashTableEntry>, NUM_TABLES> tables_;
    size_t capacity_;
    size_t size_;
    size_t max_iterations_;
    mutable size_t rehash_count_;
    mutable size_t kick_count_;
    mutable size_t max_kick_sequence_;

    /**
     * 哈希函数组
     */
    std::array<uint64_t, NUM_TABLES> hash_seeds_;

    /**
     * 哈希函数
     * @param key 键
     * @param table_id 哈希表ID (0 或 1)
     * @return 哈希值
     */
    size_t hash(uint64_t key, size_t table_id) const;

    /**
     * 初始化哈希种子
     */
    void initializeHashSeeds();

    /**
     * 执行布谷鸟踢出操作
     * @param key 要插入的键
     * @param value 要插入的值
     * @param start_table 开始的哈希表ID
     * @return 是否成功插入
     */
    bool cuckooKick(uint64_t key, uint64_t value, size_t start_table);

    /**
     * 尝试简单插入（无踢出）
     * @param key 键
     * @param value 值
     * @return 是否成功插入
     */
    bool trySimpleInsert(uint64_t key, uint64_t value);

    /**
     * 查找键在指定表中的位置
     * @param key 键
     * @param table_id 哈希表ID
     * @return 位置索引，如果未找到返回capacity_
     */
    size_t findInTable(uint64_t key, size_t table_id) const;

    /**
     * 检查是否需要重建
     * @return 是否需要重建
     */
    bool needsRebuild() const;

    /**
     * 生成新的哈希种子
     */
    void generateNewHashSeeds();
};

} // namespace SecretSharingPSI