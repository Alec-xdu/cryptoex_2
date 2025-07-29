# 基于秘密切片共享的PSI协议实现

## 项目简介

本项目实现了一个基于秘密切片共享（Secret Sharing）的私有集合求交（PSI）协议，用于替换传统APSI协议中的全同态加密（FHE）部分。该实现使用OpenSSL和C++17，提供了完整的PSI协议功能，包括TEE（可信执行环境）、Beaver三元组生成、算数秘密共享运算和多项式计算。

## 技术特性

### 核心组件

1. **秘密切片共享TEE类 (SecretSharingTEE)**
   - 种子分发机制（向A方和B方分别发送种子z、w）
   - Beaver三元组生成和验证
   - 基于OpenSSL的安全随机数生成

2. **算数秘密共享运算器 (ArithmeticSecretSharing)**
   - 基于Beaver三元组的安全乘法运算
   - 秘密共享的加法和减法运算
   - 多项式匹配计算

3. **PSI协议主类 (SecretSharingPSI)**
   - 兼容APSI架构的接口设计
   - 支持标签和非标签模式
   - 分布式和单机版本实现

4. **APSI兼容的Item处理**
   - 128位固定哈希格式
   - OPRF处理支持
   - 与APSI Item格式兼容

### 技术优势

- **替换全同态加密**: 使用秘密切片共享替代计算密集的FHE操作
- **保持隐私性**: 确保双方数据不泄露，只获得交集信息
- **高效运算**: Beaver三元组预计算，在线阶段快速执行
- **模块化设计**: 各组件独立，便于测试和扩展

## 协议原理

### TEE分发机制
```
对A方：TEE分发RNG种子z，A方生成三元组[a0, b0, c0]i
对B方：TEE分发RNG种子w，生成二元组[a1, b1]i
计算：c1_i = (a0_i + a1_i)*(b0_i + b1_i) - c0_i
```

### 秘密共享乘法
双方利用Beaver三元组进行安全乘法：
- 计算差值：d = x - a, e = y - b
- 重构差值并计算最终结果
- 保证乘法过程中不泄露原始数据

### 多项式匹配
- 构造匹配多项式：M(x) = (x - y1)(x - y2)...(x - yn)
- 在秘密共享上计算多项式求值
- 结果为0表示匹配，非0表示不匹配

## 构建与安装

### 依赖要求

- **编译器**: C++17兼容的编译器（GCC 7+, Clang 7+, MSVC 2019+）
- **CMake**: 3.12或更高版本
- **OpenSSL**: 1.1.0或更高版本
- **系统**: Linux, macOS, Windows

### 编译步骤

```bash
# 克隆项目
git clone <repository-url>
cd SecretSharingPSI

# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
cmake --build .

# 可选：运行测试
ctest

# 可选：安装
cmake --install .
```

### CMake选项

- `BUILD_EXAMPLES=ON/OFF`: 是否构建示例程序（默认ON）
- `BUILD_TESTS=ON/OFF`: 是否构建测试程序（默认ON）
- `CMAKE_BUILD_TYPE`: 构建类型（Release/Debug）

## 使用示例

### 基本PSI协议

```cpp
#include "secret_sharing_psi.h"

// 配置参数
SecretSharingPSIParams params;
params.sender_set_size = 1000;
params.receiver_set_size = 100;

// 创建PSI实例
SecretSharingPSI psi(params);
psi.initialize();

// 准备数据
std::vector<std::string> sender_items = {"item1", "item2", "item3"};
std::vector<std::string> receiver_items = {"item2", "item4"};

// 执行PSI
PSIResult result = psi.runPSI(sender_items, receiver_items);

// 处理结果
for (size_t i = 0; i < result.match_results.size(); ++i) {
    if (result.match_results[i]) {
        std::cout << receiver_items[i] << " 匹配" << std::endl;
    }
}
```

### Beaver三元组测试

```cpp
#include "secret_sharing_tee.h"
#include "arithmetic_secret_sharing.h"

// 创建TEE和算术引擎
auto tee = std::make_shared<SecretSharingTEE>();
auto arithmetic_engine = std::make_shared<ArithmeticSecretSharing>(tee);

// 初始化
arithmetic_engine->initialize(1000);

// 测试秘密共享乘法
auto share_a = arithmetic_engine->createShare(123);
auto share_b = arithmetic_engine->createShare(456);
auto share_product = arithmetic_engine->mulShares(share_a, share_b);

uint64_t result = arithmetic_engine->reconstructSecret(share_product);
std::cout << "乘法结果: " << result << std::endl;
```

## 示例程序

项目提供了以下示例程序：

1. **basic_psi_example**: 基本PSI协议演示
2. **labeled_psi_example**: 标签PSI协议演示  
3. **beaver_triplet_test**: Beaver三元组功能测试
4. **performance_test**: 性能基准测试

运行示例：
```bash
cd build/examples
./basic_psi_example
./beaver_triplet_test
```

## 性能特点

- **Beaver三元组生成**: ~1ms/1000个三元组
- **秘密共享乘法**: ~100-500ns/次操作
- **多项式计算**: 取决于项目数量和多项式度数
- **内存占用**: 线性于数据集大小和三元组数量

## 与APSI的兼容性

本实现设计为与APSI库兼容：

- **Item格式**: 使用128位哈希，与APSI Item兼容
- **OPRF支持**: 可集成APSI的OPRF实现
- **参数结构**: 类似于APSI的PSIParams设计
- **接口风格**: 遵循APSI的API设计模式

可以考虑复用APSI中的以下组件：
- Kuku布谷鸟哈希库
- OPRF实现
- 网络通信模块
- 参数配置系统

## 安全性说明

- **半诚实模型**: 假设参与方遵循协议但可能尝试推断额外信息
- **TEE信任**: 依赖可信执行环境的安全性
- **密码学安全**: 基于离散对数困难性假设
- **随机数质量**: 使用OpenSSL的密码学安全随机数生成器

## 限制与注意事项

1. **模运算**: 所有计算在大素数域上进行
2. **Beaver三元组消耗**: 每次乘法消耗一个三元组
3. **内存需求**: 需要预分配足够的三元组
4. **精度限制**: 受素数模数大小限制

## 贡献指南

欢迎贡献代码和建议：

1. Fork本项目
2. 创建特性分支
3. 提交更改
4. 发起Pull Request

## 许可证

本项目采用MIT许可证，详见LICENSE文件。

## 联系方式

如有问题或建议，请通过以下方式联系：
- 提交Issue
- 发送邮件
- 参与讨论

## 致谢

- Microsoft APSI项目提供的设计参考
- OpenSSL社区的密码学库支持
- 学术界在秘密共享和PSI领域的研究贡献