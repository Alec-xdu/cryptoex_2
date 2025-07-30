#pragma once

#include "common.h"
#include <string>
#include <vector>

namespace SecretSharingPSI {

/**
 * PSI协议中的数据项类
 * 类似APSI中的Item实现，用于存储和处理PSI数据
 */
class Item {
public:
    /**
     * 默认构造函数
     */
    Item();

    /**
     * 从字符串构造Item
     * @param data 输入字符串数据
     */
    explicit Item(const std::string& data);

    /**
     * 从字节数组构造Item
     * @param data 字节数组
     * @param size 数组大小
     */
    Item(const uint8_t* data, size_t size);

    /**
     * 从64位整数构造Item
     * @param value 64位整数值
     */
    explicit Item(uint64_t value);

    /**
     * 拷贝构造函数
     */
    Item(const Item& other);

    /**
     * 赋值操作符
     */
    Item& operator=(const Item& other);

    /**
     * 移动构造函数
     */
    Item(Item&& other) noexcept;

    /**
     * 移动赋值操作符
     */
    Item& operator=(Item&& other) noexcept;

    /**
     * 析构函数
     */
    ~Item();

    /**
     * 获取Item的哈希值
     * @return 32字节哈希值
     */
    Hash getHash() const;

    /**
     * 获取Item的64位表示
     * @return 64位整数值
     */
    uint64_t getValue() const;

    /**
     * 获取原始数据
     * @return 原始字节数据
     */
    const std::vector<uint8_t>& getRawData() const;

    /**
     * 获取数据大小
     * @return 数据字节数
     */
    size_t size() const;

    /**
     * 检查Item是否为空
     * @return 如果为空返回true
     */
    bool empty() const;

    /**
     * 清空Item数据
     */
    void clear();

    /**
     * 转换为字符串表示
     * @return 字符串表示
     */
    std::string toString() const;

    /**
     * 比较操作符
     */
    bool operator==(const Item& other) const;
    bool operator!=(const Item& other) const;
    bool operator<(const Item& other) const;

    /**
     * 计算多个Item的批量哈希
     * @param items Item向量
     * @return 哈希值向量
     */
    static std::vector<Hash> batchHash(const std::vector<Item>& items);

    /**
     * 从文件读取Item列表
     * @param filename 文件名
     * @return Item向量
     */
    static std::vector<Item> loadFromFile(const std::string& filename);

    /**
     * 将Item列表保存到文件
     * @param items Item向量
     * @param filename 文件名
     * @return 是否成功
     */
    static bool saveToFile(const std::vector<Item>& items, const std::string& filename);

private:
    std::vector<uint8_t> data_;
    mutable Hash cached_hash_;
    mutable bool hash_computed_;

    /**
     * 计算哈希值
     */
    void computeHash() const;
};

/**
 * OPRF处理后的Item结构
 */
struct OPRFItem {
    Item original_item;
    Hash oprf_output;
    uint64_t hash_value;
    
    OPRFItem() : hash_value(0) {}
    OPRFItem(const Item& item, const Hash& output, uint64_t hash_val)
        : original_item(item), oprf_output(output), hash_value(hash_val) {}
};

/**
 * 哈希表中的条目结构
 */
struct HashTableEntry {
    uint64_t key;
    uint64_t value;
    bool is_occupied;
    
    HashTableEntry() : key(0), value(0), is_occupied(false) {}
    HashTableEntry(uint64_t k, uint64_t v) : key(k), value(v), is_occupied(true) {}
};

/**
 * PSI查询结果结构
 */
struct PSIResult {
    std::vector<Item> intersection;
    size_t sender_set_size;
    size_t receiver_set_size;
    size_t intersection_size;
    
    PSIResult() : sender_set_size(0), receiver_set_size(0), intersection_size(0) {}
};

} // namespace SecretSharingPSI