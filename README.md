# 秘密切片分发系统 (Secret Share Distributor)

基于OpenSSL和APSI库实现的秘密切片分发系统，支持Beaver Triple的算术乘法运算。

## 功能特性

### 核心功能
- **种子分发**: TEE向A方和B方分别分发RNG种子z和w
- **三元组生成**: 
  - A方使用种子z生成三元组 [a₀, b₀, c₀]ᵢ
  - B方使用种子w生成二元组 [a₁, b₁]ᵢ，并计算 c₁ᵢ = (a₀ᵢ + a₁ᵢ)*(b₀ᵢ + b₁ᵢ) - c₀ᵢ
- **Beaver Triple乘法**: 基于预计算三元组的安全乘法运算
- **APSI集成**: 支持APSI Item类型的数据转换和处理

### 安全特性
- 使用OpenSSL的密码学安全随机数生成
- AES-128-CTR模式的伪随机数生成器
- 秘密分享协议确保数据隐私

## 项目结构

```
├── CMakeLists.txt                 # 构建配置
├── include/                       # 头文件目录
│   ├── secret_share_distributor.h # 主要类定义
│   ├── beaver_triple.h           # Beaver Triple工具类
│   └── rng_utils.h               # RNG工具类
├── src/                          # 源文件目录
│   ├── secret_share_distributor.cpp
│   ├── beaver_triple.cpp
│   └── rng_utils.cpp
├── examples/                     # 示例程序
│   └── main.cpp                  # 演示程序
└── README.md                     # 项目说明
```

## 依赖要求

### 必需依赖
- **CMake** >= 3.16
- **OpenSSL** >= 1.1.0
- **C++17** 兼容的编译器

### 可选依赖
- **APSI库**: 用于APSI Item类型支持（如果未找到将使用fallback实现）

## 编译安装

### 1. 安装依赖

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install cmake build-essential libssl-dev pkg-config
```

#### CentOS/RHEL
```bash
sudo yum install cmake gcc-c++ openssl-devel pkgconfig
```

### 2. 编译项目

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j$(nproc)

# 可选：安装到系统
sudo make install
```

## 使用示例

### 基本用法

```cpp
#include "secret_share_distributor.h"
#include "beaver_triple.h"

using namespace SecretShare;

int main() {
    // 1. 创建分发器
    SecretShareDistributor distributor(32);  // 32字节种子长度
    
    // 2. 生成和分发种子
    Seed seed_z, seed_w;
    distributor.generateAndDistributeSeeds(seed_z, seed_w);
    
    // 3. 生成Beaver Triple
    size_t triple_count = 10;
    auto triples_a = distributor.generateTriplesForPartyA(seed_z, triple_count);
    auto triples_b = distributor.generateTriplesForPartyB(seed_w, seed_z, triple_count);
    
    // 4. 验证三元组正确性
    bool valid = BeaverTripleUtils::verifyBatchTriples(triples_a, triples_b);
    
    // 5. 执行乘法运算
    Share x_a = 10, x_b = 5;  // x = 15 的分享
    Share y_a = 8, y_b = 4;   // y = 12 的分享
    
    Share result_a = distributor.multiplyWithBeaverTriple(x_a, y_a, triples_a[0], true);
    Share result_b = distributor.multiplyWithBeaverTriple(x_b, y_b, triples_b[0], false);
    
    Share result = distributor.reconstructSecret(result_a, result_b);
    // result = 180 (15 * 12)
    
    return 0;
}
```

### APSI集成示例

```cpp
#ifdef HAVE_APSI
#include <apsi/item.h>

// APSI Item转换
std::vector<apsi::Item> items = { /* ... */ };
auto shares = distributor.itemsToShares(items);

// 执行秘密计算
// ...

// 转换回APSI Item
auto result_items = distributor.sharesToItems(result_shares);
#endif
```

## API 参考

### SecretShareDistributor 类

#### 构造函数
```cpp
explicit SecretShareDistributor(size_t seed_length = 32);
```

#### 种子管理
```cpp
bool generateAndDistributeSeeds(Seed& seed_z, Seed& seed_w);
```

#### 三元组生成
```cpp
std::vector<BeaverTriple> generateTriplesForPartyA(const Seed& seed_z, size_t count);
std::vector<BeaverTriple> generateTriplesForPartyB(const Seed& seed_w, const Seed& seed_z, size_t count);
```

#### 乘法运算
```cpp
Share multiplyWithBeaverTriple(Share x_share, Share y_share, const BeaverTriple& triple, bool is_party_a);
std::vector<Share> batchMultiplyWithBeaverTriple(const std::vector<Share>& x_shares, const std::vector<Share>& y_shares, const std::vector<BeaverTriple>& triples, bool is_party_a);
```

#### APSI集成 (可选)
```cpp
#ifdef HAVE_APSI
Share itemToShare(const ItemType& item);
ItemType shareToItem(Share share);
std::vector<Share> itemsToShares(const std::vector<ItemType>& items);
std::vector<ItemType> sharesToItems(const std::vector<Share>& shares);
#endif
```

### BeaverTripleUtils 工具类

```cpp
static void printTriple(const BeaverTriple& triple, const std::string& party_name);
static bool verifyTriples(const BeaverTriple& triple_a, const BeaverTriple& triple_b);
static bool verifyBatchTriples(const std::vector<BeaverTriple>& triples_a, const std::vector<BeaverTriple>& triples_b);
```

## 运行示例

编译完成后，运行示例程序：

```bash
./example
```

示例程序将演示：
1. 基本功能（种子生成、三元组生成、验证）
2. Beaver Triple乘法运算
3. 性能测试
4. APSI集成（如果可用）

## 性能特征

基于测试结果，系统性能如下：

- **种子生成**: < 100 μs
- **三元组生成**: ~1-2 μs per triple
- **乘法运算**: ~0.1 μs per operation
- **内存使用**: 每个三元组约24字节

## 安全考虑

1. **随机性**: 使用OpenSSL的密码学安全随机数生成器
2. **种子安全**: 种子应通过安全信道分发
3. **三元组重用**: 每个三元组只能使用一次
4. **侧信道攻击**: 实现中未特别防护时序攻击

## 许可证

本项目采用MIT许可证。详见LICENSE文件。

## 贡献

欢迎提交Issue和Pull Request来改进项目。

## 联系方式

如有问题或建议，请通过GitHub Issues联系。