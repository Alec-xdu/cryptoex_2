#pragma once

#include "common.h"
#include "item.h"
#include "secret_sharing.h"
#include "oprf.h"
#include "hash_table.h"
#include "cuckoo_hash.h"
#include <memory>

namespace SecretSharingPSI {

/**
 * 基于秘密分享的PSI协议主类
 * 集成OPRF、哈希表和秘密分享功能
 */
class PSIProtocol {
public:
    /**
     * 协议参与方角色
     */
    enum class Role {
        SENDER,    // 发送方（使用布谷鸟哈希）
        RECEIVER,  // 接收方（使用普通哈希）
        TEE        // 可信执行环境
    };

    /**
     * 协议配置参数
     */
    struct Config {
        size_t hash_table_capacity;
        double hash_table_load_factor;
        size_t cuckoo_max_iterations;
        size_t beaver_triple_count;
        
        Config() : hash_table_capacity(4096), hash_table_load_factor(0.75),
                  cuckoo_max_iterations(8), beaver_triple_count(1000) {}
    };

    /**
     * 构造函数
     * @param role 参与方角色
     * @param config 配置参数
     */
    explicit PSIProtocol(Role role, const Config& config = Config());

    /**
     * 析构函数
     */
    ~PSIProtocol();

    /**
     * 初始化协议
     * @return 是否成功初始化
     */
    bool initialize();

    /**
     * TEE: 生成并分发种子
     * @param seed_a 给参与方A的种子
     * @param seed_b 给参与方B的种子
     * @return 是否成功生成种子
     */
    bool generateSeeds(Seed& seed_a, Seed& seed_b);

    /**
     * TEE: 为参与方B生成预处理数据
     * @param seed_a 参与方A的种子
     * @param seed_b 参与方B的种子
     * @return 参与方B的预处理数据
     */
    SecretSharing::PartyBData generatePartyBData(const Seed& seed_a, const Seed& seed_b);

    /**
     * 参与方初始化秘密分享
     * @param seed 从TEE接收的种子
     * @param party_b_data 参与方B的预处理数据（仅B方需要）
     * @return 是否成功初始化
     */
    bool initializeSecretSharing(const Seed& seed, 
                                const SecretSharing::PartyBData* party_b_data = nullptr);

    /**
     * 设置输入数据集
     * @param items 输入数据项
     * @return 是否成功设置
     */
    bool setInputSet(const std::vector<Item>& items);

    /**
     * 从文件加载输入数据集
     * @param filename 文件名
     * @return 是否成功加载
     */
    bool loadInputSet(const std::string& filename);

    /**
     * 执行OPRF处理
     * @return 是否成功处理
     */
    bool processOPRF();

    /**
     * 构建哈希表
     * @return 是否成功构建
     */
    bool buildHashTable();

    /**
     * 发送方：获取查询数据
     * @return 查询用的哈希值列表
     */
    std::vector<uint64_t> getSenderQueries() const;

    /**
     * 接收方：处理查询并返回结果
     * @param queries 查询列表
     * @return 查询结果
     */
    std::vector<std::pair<uint64_t, uint64_t>> processQueries(const std::vector<uint64_t>& queries);

    /**
     * 发送方：处理查询结果，计算交集
     * @param query_results 查询结果
     * @return PSI结果
     */
    PSIResult computeIntersection(const std::vector<std::pair<uint64_t, uint64_t>>& query_results);

    /**
     * 执行完整的PSI协议（用于测试）
     * @param other_protocol 另一个参与方的协议实例
     * @return PSI结果
     */
    PSIResult runProtocol(PSIProtocol& other_protocol);

    /**
     * 获取统计信息
     */
    struct Statistics {
        size_t input_set_size;
        size_t oprf_processed_count;
        size_t hash_table_size;
        size_t intersection_size;
        double hash_table_load_factor;
        size_t secret_sharing_operations;
    };
    Statistics getStatistics() const;

    /**
     * 获取当前角色
     * @return 参与方角色
     */
    Role getRole() const;

    /**
     * 检查协议是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const;

private:
    Role role_;
    Config config_;
    bool initialized_;

    // 核心组件
    std::unique_ptr<SecretSharing> secret_sharing_;
    std::unique_ptr<OPRF> oprf_;
    std::unique_ptr<HashTable> hash_table_;
    std::unique_ptr<CuckooHashTable> cuckoo_table_;

    // 数据存储
    std::vector<Item> input_items_;
    std::vector<OPRFItem> oprf_items_;

    // 统计信息
    mutable Statistics stats_;

    /**
     * 初始化OPRF
     * @return 是否成功初始化
     */
    bool initializeOPRF();

    /**
     * 初始化哈希表
     * @return 是否成功初始化
     */
    bool initializeHashTables();

    /**
     * 更新统计信息
     */
    void updateStatistics() const;

    /**
     * 验证协议状态
     * @param required_state 需要的状态描述
     * @return 是否满足状态要求
     */
    bool validateState(const std::string& required_state) const;
};

} // namespace SecretSharingPSI