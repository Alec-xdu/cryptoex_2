# Secret Sharing PSI Protocol

基于秘密分享的私有集合求交（PSI）协议实现，使用C++和OpenSSL库开发。该协议替换了传统APSI中的全同态加密部分，采用基于海狸三元组的秘密分享技术。

## 项目概述

本项目实现了一个新的PSI协议，具有以下特点：

- **秘密分享**: 使用海狸三元组替代全同态加密
- **TEE支持**: 可信执行环境负责种子生成和预处理
- **高效哈希**: 发送方使用布谷鸟哈希，接收方使用普通哈希
- **OPRF集成**: 支持不经意伪随机函数处理
- **模块化设计**: 各组件独立，易于扩展和测试

## 系统架构

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│       TEE       │    │    Sender       │    │    Receiver     │
│                 │    │                 │    │                 │
│ ┌─────────────┐ │    │ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │ Seed Gen    │ │    │ │ Cuckoo Hash │ │    │ │ Hash Table  │ │
│ │ Triple Gen  │ │    │ │ OPRF        │ │    │ │ OPRF        │ │
│ │ Preprocess  │ │    │ │ SecretShare │ │    │ │ SecretShare │ │
│ └─────────────┘ │    │ └─────────────┘ │    │ └─────────────┘ │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 核心组件

### 1. SecretSharing 类
- **功能**: 实现基于海狸三元组的秘密分享
- **特性**: 
  - TEE生成种子和预处理数据
  - 参与方A获得三元组 [a₀, b₀, c₀]
  - 参与方B获得二元组 [a₁, b₁] 和计算值 c₁
  - 支持安全乘法协议

### 2. Item 类
- **功能**: PSI协议中的数据项管理
- **特性**:
  - 支持多种数据类型输入
  - SHA-256哈希计算
  - 批量处理和文件I/O

### 3. OPRF 类
- **功能**: 不经意伪随机函数实现
- **特性**:
  - HMAC-based简化实现
  - 支持盲化和去盲化
  - 批量处理优化

### 4. 哈希表组件
- **HashTable**: 普通哈希表，用于接收方
- **CuckooHashTable**: 布谷鸟哈希表，用于发送方
- 两者都支持批量操作和性能统计

### 5. PSIProtocol 类
- **功能**: 协议主控制器
- **特性**:
  - 集成所有组件
  - 支持三种角色：Sender、Receiver、TEE
  - 完整的协议流程管理

## 编译和安装

### 依赖要求

- C++17 编译器 (GCC 7+ 或 Clang 5+)
- CMake 3.12+
- OpenSSL 1.1.1+
- pthread

### 编译步骤

```bash
# 克隆项目
git clone <repository-url>
cd SecretSharingPSI

# 创建构建目录
mkdir build && cd build

# 配置和编译
cmake ..
make -j$(nproc)

# 运行测试
./psi_test

# 运行示例
./psi_example
```

### Ubuntu/Debian 安装依赖
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libssl-dev
```

### CentOS/RHEL 安装依赖
```bash
sudo yum install gcc-c++ cmake openssl-devel
```

## 使用示例

### 基本PSI协议使用

```cpp
#include "psi_protocol.h"
using namespace SecretSharingPSI;

int main() {
    // 创建协议实例
    PSIProtocol::Config config;
    config.hash_table_capacity = 1024;
    config.beaver_triple_count = 500;
    
    PSIProtocol sender(PSIProtocol::Role::SENDER, config);
    PSIProtocol receiver(PSIProtocol::Role::RECEIVER, config);
    PSIProtocol tee(PSIProtocol::Role::TEE, config);
    
    // 初始化所有协议
    sender.initialize();
    receiver.initialize();
    tee.initialize();
    
    // TEE生成种子和预处理数据
    Seed seed_a, seed_b;
    tee.generateSeeds(seed_a, seed_b);
    auto party_b_data = tee.generatePartyBData(seed_a, seed_b);
    
    // 初始化秘密分享
    sender.initializeSecretSharing(seed_a);
    receiver.initializeSecretSharing(seed_b, &party_b_data);
    
    // 设置输入数据集
    std::vector<Item> sender_set = {"item1", "item2", "item3"};
    std::vector<Item> receiver_set = {"item2", "item3", "item4"};
    
    sender.setInputSet(sender_set);
    receiver.setInputSet(receiver_set);
    
    // 执行PSI协议
    PSIResult result = sender.runProtocol(receiver);
    
    // 输出结果
    std::cout << "Intersection size: " << result.intersection_size << std::endl;
    
    return 0;
}
```

### 秘密分享独立使用

```cpp
#include "secret_sharing.h"
using namespace SecretSharingPSI;

int main() {
    // TEE生成种子
    SecretSharing tee;
    Seed seed_a, seed_b;
    tee.generateSeeds(seed_a, seed_b);
    
    // TEE为参与方B生成预处理数据
    auto party_b_data = tee.generateDataForB(seed_a, seed_b, 100);
    
    // 初始化参与方
    SecretSharing party_a, party_b;
    party_a.initializePartyA(seed_a, 100);
    party_b.initializePartyB(seed_b, party_b_data.pairs, party_b_data.c1_values);
    
    // 使用海狸三元组进行安全乘法
    uint64_t x_share_a = 123, y_share_a = 456;
    uint64_t x_share_b = 789, y_share_b = 321;
    
    uint64_t result_a = party_a.secureMultiplyA(x_share_a, y_share_a, 0);
    uint64_t result_b = party_b.secureMultiplyB(x_share_b, y_share_b, 0);
    
    return 0;
}
```

## 协议流程

1. **初始化阶段**
   - TEE生成随机种子 seed_a 和 seed_b
   - TEE使用种子生成海狸三元组和预处理数据
   - 参与方A和B分别初始化本地状态

2. **数据预处理**
   - 双方对输入数据执行OPRF处理
   - 发送方将OPRF结果插入布谷鸟哈希表
   - 接收方将OPRF结果插入普通哈希表

3. **查询阶段**
   - 发送方生成查询请求
   - 接收方处理查询并返回结果
   - 发送方根据查询结果计算交集

4. **结果输出**
   - 返回交集大小和具体元素
   - 提供详细的性能统计信息

## 性能特点

- **时间复杂度**: O(n + m)，其中n和m分别是两个集合的大小
- **空间复杂度**: O(n + m)
- **通信复杂度**: O(n + m)
- **安全性**: 基于海狸三元组的半诚实安全模型

## 测试和验证

项目包含完整的测试套件：

```bash
# 运行所有测试
./psi_test

# 运行性能基准测试
./psi_example
```

测试覆盖：
- 单元测试：各个组件的独立功能测试
- 集成测试：完整协议流程测试
- 性能测试：哈希表和协议性能评估
- 正确性验证：交集计算结果验证

## 配置参数

主要配置参数及其默认值：

```cpp
struct Config {
    size_t hash_table_capacity = 4096;      // 哈希表容量
    double hash_table_load_factor = 0.75;   // 负载因子
    size_t cuckoo_max_iterations = 8;       // 布谷鸟哈希最大迭代次数
    size_t beaver_triple_count = 1000;      // 海狸三元组数量
};
```

## 安全考虑

1. **半诚实安全模型**: 假设参与方遵循协议但可能尝试推断额外信息
2. **随机数生成**: 使用OpenSSL的加密安全随机数生成器
3. **哈希函数**: 使用SHA-256保证数据完整性
4. **秘密分享**: 基于加法秘密分享和海狸三元组

## 扩展和定制

项目采用模块化设计，支持以下扩展：

1. **新的哈希表实现**: 继承基类并实现接口
2. **不同的OPRF方案**: 替换OPRF类实现
3. **网络通信**: 添加网络层支持分布式部署
4. **性能优化**: 并行化处理和SIMD优化

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 贡献指南

欢迎提交问题报告和改进建议：

1. Fork项目
2. 创建特性分支
3. 提交更改
4. 创建Pull Request

## 联系信息

如有问题或建议，请通过以下方式联系：

- GitHub Issues: 项目问题跟踪
- Email: [项目维护者邮箱]

## 参考文献

1. APSI Protocol: [Microsoft APSI](https://github.com/microsoft/APSI)
2. Beaver Triples: "Efficient Multiparty Protocols Using Circuit Randomization"
3. Cuckoo Hashing: "Cuckoo Hashing" by Pagh and Rodler
4. OPRF: "The OPRF Protocol" RFC