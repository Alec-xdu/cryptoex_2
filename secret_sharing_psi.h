#ifndef SECRET_SHARING_PSI_H
#define SECRET_SHARING_PSI_H

#include "secret_sharing_tee.h"
#include "arithmetic_secret_sharing.h"
#include "psi_item.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

// 兼容APSI的PSI参数结构
struct SecretSharingPSIParams {
    size_t sender_set_size;
    size_t receiver_set_size;
    size_t bin_bundle_size;        // 每个bin bundle的大小
    size_t num_hash_functions;     // 布谷鸟哈希函数数量  
    size_t beaver_triplet_count;   // Beaver三元组数量
    bool use_labeled_mode;          // 是否使用标签模式
    size_t max_label_length;        // 最大标签长度
    
    SecretSharingPSIParams()
        : sender_set_size(1000)
        , receiver_set_size(100) 
        , bin_bundle_size(64)
        , num_hash_functions(3)
        , beaver_triplet_count(10000)
        , use_labeled_mode(false)
        , max_label_length(0) {}
};

// PSI查询结果
struct PSIResult {
    std::vector<bool> match_results;     // 匹配结果
    std::vector<std::string> labels;     // 标签（如果是标签模式）
    bool success;
    
    PSIResult() : success(false) {}
};

// 发送方数据库类（类似APSI的SenderDB）
class SecretSharingSenderDB {
private:
    std::vector<PSIItem> items_;
    std::unordered_map<std::string, std::string> item_labels_;  // item hash -> label
    SecretSharingPSIParams params_;
    std::shared_ptr<OPRFProcessor> oprf_processor_;
    bool is_initialized_;
    
    // 分组存储items用于bin bundles
    std::vector<std::vector<uint64_t>> bin_bundles_;
    
public:
    SecretSharingSenderDB(const SecretSharingPSIParams& params);
    ~SecretSharingSenderDB();
    
    // 设置数据
    bool setData(const std::vector<std::string>& items);
    bool setData(const std::vector<PSIItem>& items);
    bool setLabeledData(const std::vector<std::pair<std::string, std::string>>& item_label_pairs);
    
    // 添加/删除单个项目
    bool insertItem(const std::string& item, const std::string& label = "");
    bool removeItem(const std::string& item);
    
    // 获取信息
    size_t size() const { return items_.size(); }
    const SecretSharingPSIParams& getParams() const { return params_; }
    bool isInitialized() const { return is_initialized_; }
    
    // 获取OPRF处理器
    std::shared_ptr<OPRFProcessor> getOPRFProcessor() const { return oprf_processor_; }
    
    // 获取分组的bin bundles
    const std::vector<std::vector<uint64_t>>& getBinBundles() const { return bin_bundles_; }
    
    // 处理查询（内部使用）
    std::vector<ArithmeticSecretSharing::PolynomialResult> processQuery(
        const std::vector<ArithmeticSecretSharing::SharePair>& query_shares,
        std::shared_ptr<ArithmeticSecretSharing> arithmetic_engine
    );
    
private:
    void organizeBinBundles();
    std::vector<uint64_t> processItemsWithOPRF(const std::vector<PSIItem>& items);
};

// 接收方类（类似APSI的Receiver）
class SecretSharingReceiver {
private:
    SecretSharingPSIParams params_;
    std::vector<PSIItem> query_items_;
    std::shared_ptr<OPRFProcessor> oprf_processor_;
    bool is_initialized_;
    
public:
    SecretSharingReceiver(const SecretSharingPSIParams& params);
    ~SecretSharingReceiver();
    
    // 设置查询数据
    bool setQuery(const std::vector<std::string>& items);
    bool setQuery(const std::vector<PSIItem>& items);
    
    // 获取信息
    size_t getQuerySize() const { return query_items_.size(); }
    const SecretSharingPSIParams& getParams() const { return params_; }
    bool isInitialized() const { return is_initialized_; }
    
    // 创建查询份额（内部使用）
    std::vector<ArithmeticSecretSharing::SharePair> createQueryShares(
        std::shared_ptr<ArithmeticSecretSharing> arithmetic_engine,
        const std::vector<uint8_t>& oprf_key
    );
    
    // 处理查询结果
    PSIResult processResults(
        const std::vector<ArithmeticSecretSharing::PolynomialResult>& polynomial_results,
        std::shared_ptr<ArithmeticSecretSharing> arithmetic_engine
    );
    
private:
    std::vector<uint64_t> processQueryWithOPRF(const std::vector<uint8_t>& oprf_key);
};

// 发送方类（类似APSI的Sender）
class SecretSharingSender {
private:
    std::shared_ptr<SecretSharingSenderDB> sender_db_;
    std::shared_ptr<SecretSharingTEE> tee_;
    bool is_initialized_;
    
public:
    SecretSharingSender(std::shared_ptr<SecretSharingSenderDB> sender_db);
    ~SecretSharingSender();
    
    // 初始化TEE和Beaver三元组
    bool initialize();
    
    // 获取OPRF密钥
    std::vector<uint8_t> getOPRFKey() const;
    
    // 处理PSI查询
    std::vector<ArithmeticSecretSharing::PolynomialResult> handleQuery(
        const std::vector<ArithmeticSecretSharing::SharePair>& query_shares
    );
    
    // 获取TEE数据（用于分发给各方）
    SecretSharingTEE::PartyAData getPartyAData() const;
    SecretSharingTEE::PartyBData getPartyBData() const;
    
    bool isInitialized() const { return is_initialized_; }
    
private:
    bool setupTEE();
};

// 主PSI协议类
class SecretSharingPSI {
private:
    SecretSharingPSIParams params_;
    std::shared_ptr<SecretSharingTEE> tee_;
    std::shared_ptr<ArithmeticSecretSharing> arithmetic_engine_;
    bool is_initialized_;
    
public:
    SecretSharingPSI(const SecretSharingPSIParams& params = SecretSharingPSIParams());
    ~SecretSharingPSI();
    
    // 初始化协议
    bool initialize();
    
    // 运行PSI协议（单机版本，用于测试）
    PSIResult runPSI(
        const std::vector<std::string>& sender_items,
        const std::vector<std::string>& receiver_items
    );
    
    // 运行标签PSI协议
    PSIResult runLabeledPSI(
        const std::vector<std::pair<std::string, std::string>>& sender_item_label_pairs,
        const std::vector<std::string>& receiver_items
    );
    
    // 分布式版本接口
    struct DistributedPSIContext {
        std::shared_ptr<SecretSharingSender> sender;
        std::shared_ptr<SecretSharingReceiver> receiver;
        std::shared_ptr<ArithmeticSecretSharing> arithmetic_engine_a;  // A方算术引擎
        std::shared_ptr<ArithmeticSecretSharing> arithmetic_engine_b;  // B方算术引擎
    };
    
    // 创建分布式PSI上下文
    DistributedPSIContext createDistributedContext(
        const std::vector<std::string>& sender_items,
        const std::vector<std::string>& receiver_items
    );
    
    // 执行分布式PSI
    PSIResult executeDistributedPSI(DistributedPSIContext& context);
    
    // 获取参数
    const SecretSharingPSIParams& getParams() const { return params_; }
    bool isInitialized() const { return is_initialized_; }
    
    // 静态工具函数
    static std::vector<std::string> generateRandomItems(size_t count, size_t item_length = 16);
    static std::vector<std::pair<std::string, std::string>> generateRandomLabeledItems(
        size_t count, size_t item_length = 16, size_t label_length = 32);
    
private:
    bool setupComponents();
};

#endif // SECRET_SHARING_PSI_H