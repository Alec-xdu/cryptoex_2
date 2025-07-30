#pragma once

#include "common.h"
#include "item.h"
#include <vector>

namespace SecretSharingPSI {

/**
 * 简化的OPRF (Oblivious Pseudorandom Function) 实现
 * 用于PSI协议中的数据处理
 */
class OPRF {
public:
    /**
     * 构造函数
     */
    OPRF();

    /**
     * 析构函数
     */
    ~OPRF();

    /**
     * 生成OPRF密钥
     * @return 是否成功生成密钥
     */
    bool generateKey();

    /**
     * 设置OPRF密钥
     * @param key 密钥数据
     * @return 是否成功设置密钥
     */
    bool setKey(const std::vector<uint8_t>& key);

    /**
     * 获取OPRF密钥
     * @return 密钥数据
     */
    std::vector<uint8_t> getKey() const;

    /**
     * 对单个item执行OPRF
     * @param item 输入item
     * @return OPRF处理后的结果
     */
    OPRFItem processItem(const Item& item) const;

    /**
     * 批量处理items
     * @param items 输入items
     * @return OPRF处理后的结果向量
     */
    std::vector<OPRFItem> batchProcess(const std::vector<Item>& items) const;

    /**
     * 发送方处理 - 生成盲化值
     * @param items 输入items
     * @return 盲化后的值
     */
    std::vector<Hash> senderBlind(const std::vector<Item>& items) const;

    /**
     * 接收方处理 - 对盲化值应用OPRF
     * @param blinded_items 盲化的items
     * @return OPRF输出
     */
    std::vector<Hash> receiverEvaluate(const std::vector<Hash>& blinded_items) const;

    /**
     * 发送方去盲化
     * @param oprf_outputs OPRF输出
     * @param original_items 原始items
     * @return 最终的OPRF结果
     */
    std::vector<OPRFItem> senderUnblind(const std::vector<Hash>& oprf_outputs, 
                                       const std::vector<Item>& original_items) const;

    /**
     * 简化的HMAC-based OPRF
     * @param input 输入数据
     * @return OPRF输出
     */
    Hash simpleOPRF(const std::vector<uint8_t>& input) const;

    /**
     * 检查OPRF是否已初始化
     * @return 是否已初始化
     */
    bool isInitialized() const;

private:
    std::vector<uint8_t> key_;
    bool initialized_;

    /**
     * 内部哈希函数
     * @param data 输入数据
     * @param key 密钥
     * @return 哈希输出
     */
    Hash hmacHash(const std::vector<uint8_t>& data, const std::vector<uint8_t>& key) const;

    /**
     * 生成随机盲化因子
     * @return 盲化因子
     */
    std::vector<uint8_t> generateBlindingFactor() const;

    /**
     * 应用盲化
     * @param data 原始数据
     * @param blinding_factor 盲化因子
     * @return 盲化后的数据
     */
    std::vector<uint8_t> applyBlinding(const std::vector<uint8_t>& data, 
                                      const std::vector<uint8_t>& blinding_factor) const;

    /**
     * 去除盲化
     * @param blinded_data 盲化的数据
     * @param blinding_factor 盲化因子
     * @return 去盲化后的数据
     */
    std::vector<uint8_t> removeBlinding(const std::vector<uint8_t>& blinded_data,
                                       const std::vector<uint8_t>& blinding_factor) const;
};

} // namespace SecretSharingPSI