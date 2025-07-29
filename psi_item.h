#ifndef PSI_ITEM_H
#define PSI_ITEM_H

#include <vector>
#include <string>
#include <cstdint>
#include <array>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

// PSI协议中的数据项 - 兼容APSI Item格式
class PSIItem {
public:
    // APSI兼容的固定大小哈希值（128位）
    static constexpr size_t ITEM_HASH_SIZE = 16; // 128位
    using ItemHash = std::array<uint8_t, ITEM_HASH_SIZE>;
    
private:
    std::vector<uint8_t> data_;
    ItemHash item_hash_;
    bool hash_computed_;
    
public:
    PSIItem();
    PSIItem(const std::string& str_data);
    PSIItem(const std::vector<uint8_t>& raw_data);
    PSIItem(const char* data, size_t length);
    PSIItem(const PSIItem& other);
    PSIItem& operator=(const PSIItem& other);
    
    // 获取原始数据
    const std::vector<uint8_t>& getData() const { return data_; }
    
    // 获取APSI兼容的128位哈希值
    const ItemHash& getItemHash() const;
    
    // 获取哈希值作为vector（向后兼容）
    std::vector<uint8_t> getHash() const;
    
    // 设置数据并计算哈希
    void setData(const std::string& str_data);
    void setData(const std::vector<uint8_t>& raw_data);
    void setData(const char* data, size_t length);
    
    // 获取数据大小
    size_t size() const { return data_.size(); }
    
    // 检查是否为空
    bool empty() const { return data_.empty(); }
    
    // 清空数据
    void clear();
    
    // 比较操作（基于哈希值）
    bool operator==(const PSIItem& other) const;
    bool operator!=(const PSIItem& other) const;
    bool operator<(const PSIItem& other) const;
    
    // 转换为字符串表示
    std::string toString() const;
    std::string toHexString() const;
    
    // 静态方法：从十六进制字符串创建Item
    static PSIItem fromHexString(const std::string& hex_str);
    
private:
    void computeHash();
};

// OPRF（Oblivious Pseudo-Random Function）处理器
class OPRFProcessor {
private:
    std::vector<uint8_t> key_;
    static const size_t KEY_SIZE = 32; // 256位密钥
    
public:
    OPRFProcessor();
    explicit OPRFProcessor(const std::vector<uint8_t>& key);
    
    // 生成新的OPRF密钥
    void generateKey();
    
    // 设置OPRF密钥
    void setKey(const std::vector<uint8_t>& key);
    
    // 获取OPRF密钥
    const std::vector<uint8_t>& getKey() const { return key_; }
    
    // 对单个Item进行OPRF处理
    std::vector<uint8_t> processItem(const PSIItem& item) const;
    
    // 对Item列表进行批量OPRF处理
    std::vector<std::vector<uint8_t>> processItems(const std::vector<PSIItem>& items) const;
    
    // HMAC-SHA256实现
    std::vector<uint8_t> hmacSha256(const std::vector<uint8_t>& data) const;
};

#endif // PSI_ITEM_H