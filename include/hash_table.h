#pragma once

#include "common.h"
#include "item.h"
#include <vector>
#include <functional>

namespace SecretSharingPSI {

/**
 * 普通哈希表实现
 * 用于PSI协议中接收方的数据存储
 */
class HashTable {
public:
    /**
     * 构造函数
     * @param capacity 哈希表容量
     * @param load_factor 负载因子
     */
    explicit HashTable(size_t capacity = 1024, double load_factor = 0.75);

    /**
     * 析构函数
     */
    ~HashTable();

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
        size_t collision_count;
        size_t max_probe_distance;
        double average_probe_distance;
    };
    Statistics getStatistics() const;

    /**
     * 调整哈希表大小
     * @param new_capacity 新容量
     */
    void resize(size_t new_capacity);

private:
    std::vector<HashTableEntry> table_;
    size_t capacity_;
    size_t size_;
    double max_load_factor_;
    mutable size_t collision_count_;

    /**
     * 哈希函数
     * @param key 键
     * @return 哈希值
     */
    size_t hash(uint64_t key) const;

    /**
     * 第二个哈希函数（用于双重哈希）
     * @param key 键
     * @return 哈希值
     */
    size_t hash2(uint64_t key) const;

    /**
     * 查找键的位置
     * @param key 键
     * @return 位置索引，如果未找到返回capacity_
     */
    size_t findPosition(uint64_t key) const;

    /**
     * 查找插入位置
     * @param key 键
     * @return 可插入的位置索引
     */
    size_t findInsertPosition(uint64_t key) const;

    /**
     * 检查是否需要扩容
     * @return 是否需要扩容
     */
    bool needsResize() const;
};

} // namespace SecretSharingPSI