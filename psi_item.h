#ifndef PSI_ITEM_H
#define PSI_ITEM_H

#include <vector>
#include <string>
#include <cstdint>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

// PSI协议中的数据项
class PSIItem {
private:
    std::vector<uint8_t> data_;
    std::vector<uint8_t> hash_;
    
public:
    PSIItem();
    PSIItem(const std::string& str_data);
    PSIItem(const std::vector<uint8_t>& raw_data);
    PSIItem(const PSIItem& other);
    PSIItem& operator=(const PSIItem& other);
    
    // 获取原始数据
    const std::vector<uint8_t>& getData() const { return data_; }
    
    // 获取哈希值
    const std::vector<uint8_t>& getHash() const { return hash_; }
    
    // 设置数据并计算哈希
    void setData(const std::string& str_data);
    void setData(const std::vector<uint8_t>& raw_data);
    
    // 比较操作
    bool operator==(const PSIItem& other) const;
    bool operator!=(const PSIItem& other) const;
    bool operator<(const PSIItem& other) const;
    
    // 转换为字符串表示
    std::string toString() const;
    
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