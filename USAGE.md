# 使用指南

## 快速开始

### 1. 编译项目

```bash
# 安装依赖 (Ubuntu/Debian)
sudo apt-get install cmake build-essential libssl-dev pkg-config

# 编译
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. 运行示例

```bash
./example
```

## 核心概念

### 秘密分享 (Secret Sharing)
- 将秘密值分割成多个分享，单个分享无法恢复原始秘密
- 本系统使用加法秘密分享：`secret = share_a + share_b`

### Beaver Triple
- 预计算的三元组 `(a, b, c)`，其中 `c = a * b`
- 用于安全地计算两个秘密分享值的乘积
- 每个三元组只能使用一次

## 基本使用流程

### 1. 创建分发器

```cpp
#include "secret_share_distributor.h"
using namespace SecretShare;

// 创建32字节种子长度的分发器
SecretShareDistributor distributor(32);
```

### 2. 生成和分发种子

```cpp
Seed seed_z, seed_w;  // A方和B方的种子
bool success = distributor.generateAndDistributeSeeds(seed_z, seed_w);
```

### 3. 生成Beaver Triple

```cpp
size_t triple_count = 10;

// A方生成三元组
auto triples_a = distributor.generateTriplesForPartyA(seed_z, triple_count);

// B方生成三元组（需要A方种子来计算c1）
auto triples_b = distributor.generateTriplesForPartyB(seed_w, seed_z, triple_count);
```

### 4. 验证三元组正确性

```cpp
#include "beaver_triple.h"

bool valid = BeaverTripleUtils::verifyBatchTriples(triples_a, triples_b);
if (valid) {
    std::cout << "三元组验证通过" << std::endl;
}
```

### 5. 执行安全乘法

#### 方法一：完整的两方协议

```cpp
// 原始数据
Share x = 15, y = 12;

// 生成秘密分享
Share x_a, x_b, y_a, y_b;
BeaverTripleUtils::generateTestShares(x, x_a, x_b);
BeaverTripleUtils::generateTestShares(y, y_a, y_b);

// 执行乘法
Share result = distributor.twoPartyMultiplyWithBeaverTriple(
    x_a, x_b, y_a, y_b, triples_a[0], triples_b[0]);

// result = 180 (15 * 12)
```

#### 方法二：单方计算（需要通信）

```cpp
// A方计算
Share result_a = distributor.multiplyWithBeaverTriple(x_a, y_a, triples_a[0], true);

// B方计算
Share result_b = distributor.multiplyWithBeaverTriple(x_b, y_b, triples_b[0], false);

// 重建结果
Share result = distributor.reconstructSecret(result_a, result_b);
```

## 高级功能

### 批量乘法运算

```cpp
std::vector<Share> x_values = {10, 20, 30};
std::vector<Share> y_values = {5, 4, 3};

// 生成分享
std::vector<Share> x_shares_a, x_shares_b;
std::vector<Share> y_shares_a, y_shares_b;
BeaverTripleUtils::generateBatchTestShares(x_values, x_shares_a, x_shares_b);
BeaverTripleUtils::generateBatchTestShares(y_values, y_shares_a, y_shares_b);

// 批量乘法
auto results_a = distributor.batchMultiplyWithBeaverTriple(
    x_shares_a, y_shares_a, triples_a, true);
auto results_b = distributor.batchMultiplyWithBeaverTriple(
    x_shares_b, y_shares_b, triples_b, false);

// 重建结果
for (size_t i = 0; i < results_a.size(); ++i) {
    Share result = distributor.reconstructSecret(results_a[i], results_b[i]);
    std::cout << "结果[" << i << "] = " << result << std::endl;
}
```

### APSI集成（如果可用）

```cpp
#ifdef HAVE_APSI
// APSI Item到Share的转换
std::vector<apsi::Item> items = { /* ... */ };
auto shares = distributor.itemsToShares(items);

// 执行计算
// ...

// 转换回APSI Item
auto result_items = distributor.sharesToItems(result_shares);
#endif
```

## 性能优化建议

### 1. 预计算三元组
```cpp
// 预先生成大量三元组
size_t large_count = 100000;
auto triples_a = distributor.generateTriplesForPartyA(seed_z, large_count);
auto triples_b = distributor.generateTriplesForPartyB(seed_w, seed_z, large_count);
```

### 2. 批量操作
```cpp
// 使用批量API而不是单个操作
auto results = distributor.batchMultiplyWithBeaverTriple(x_shares, y_shares, triples, true);
```

### 3. 内存管理
```cpp
// 及时释放不需要的三元组
triples_a.clear();
triples_a.shrink_to_fit();
```

## 安全注意事项

### 1. 种子安全
- 种子应通过安全信道分发
- 不同的计算会话应使用不同的种子
- 种子长度至少为32字节

### 2. 三元组使用
- 每个三元组只能使用一次
- 使用后应立即删除
- 不要重复使用已消耗的三元组

### 3. 通信安全
- 在真实的两方协议中，d和e的值需要通过安全信道交换
- 使用TLS或其他加密协议保护网络通信

## 调试和测试

### 1. 启用调试输出
```cpp
// 打印三元组信息
BeaverTripleUtils::printTriple(triple, "Party A");

// 验证三元组
bool valid = BeaverTripleUtils::verifyTriples(triple_a, triple_b);
```

### 2. 性能测试
```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
// 执行操作
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
std::cout << "耗时: " << duration.count() << " μs" << std::endl;
```

### 3. 内存使用监控
```cpp
// 估算内存使用
size_t memory_per_triple = sizeof(BeaverTriple);  // 约24字节
size_t total_memory = triple_count * memory_per_triple;
```

## 常见问题

### Q: 为什么乘法结果不正确？
A: 检查以下几点：
- 确保使用了正确的三元组对
- 验证三元组的正确性
- 确保输入是正确的秘密分享

### Q: 如何提高性能？
A: 考虑以下优化：
- 使用批量操作
- 预计算三元组
- 使用更大的种子缓存

### Q: 如何集成到现有项目？
A: 参考以下步骤：
- 包含必要的头文件
- 链接libsecret_share_distributor.a
- 确保OpenSSL库可用

## 扩展功能

本系统可以扩展支持：
- 更多的算术运算（加法、减法已内置支持）
- 不同的秘密分享方案
- 网络通信模块
- 更高级的安全协议

详细的API文档请参考头文件中的注释。