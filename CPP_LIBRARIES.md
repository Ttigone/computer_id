# 纯 C++ 授权系统库依赖说明

## 🎯 目标

提供**不依赖 Qt** 的纯 C++ 授权系统，只在 Qt 项目中使用 UI，其他功能使用标准 C++ 库。

---

## 📚 所需库清单

### 1. **OpenSSL** (必需)

**功能**: 加密、哈希、HMAC、Base64

**安装方法**:

#### Windows (Visual Studio)
```bash
# 方法 1: vcpkg（推荐）
vcpkg install openssl:x64-windows

# 方法 2: 预编译包
# 下载: https://slproweb.com/products/Win32OpenSSL.html
# 选择 Win64 OpenSSL v3.x.x
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install libssl-dev

# CentOS/RHEL
sudo yum install openssl-devel
```

#### macOS
```bash
brew install openssl
```

**Visual Studio 项目配置**:
```xml
<!-- 在 .vcxproj 中添加 -->
<PropertyGroup>
  <IncludePath>C:\vcpkg\installed\x64-windows\include;$(IncludePath)</IncludePath>
  <LibraryPath>C:\vcpkg\installed\x64-windows\lib;$(LibraryPath)</LibraryPath>
</PropertyGroup>

<ItemDefinitionGroup>
  <Link>
    <AdditionalDependencies>libssl.lib;libcrypto.lib;%(AdditionalDependencies)</AdditionalDependencies>
  </Link>
</ItemDefinitionGroup>
```

---

### 2. **libcurl** (必需)

**功能**: HTTP/HTTPS 网络通信

**安装方法**:

#### Windows
```bash
# vcpkg（推荐）
vcpkg install curl:x64-windows

# 或下载预编译包
# https://curl.se/windows/
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# CentOS/RHEL
sudo yum install libcurl-devel
```

#### macOS
```bash
brew install curl
```

**Visual Studio 配置**:
```xml
<ItemDefinitionGroup>
  <Link>
    <AdditionalDependencies>libcurl.lib;%(AdditionalDependencies)</AdditionalDependencies>
  </Link>
</ItemDefinitionGroup>
```

---

### 3. **JSON 库** (推荐但可选)

目前代码使用简化的 JSON 解析，生产环境建议使用专业库。

#### 选项 A: nlohmann/json（推荐，Header-Only）

```bash
# vcpkg
vcpkg install nlohmann-json:x64-windows

# 或手动下载单个头文件
# https://github.com/nlohmann/json
```

**使用示例**:
```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;

json j;
j["machine_code"] = machineCode;
std::string jsonStr = j.dump();
```

#### 选项 B: RapidJSON（Header-Only）

```bash
# vcpkg
vcpkg install rapidjson:x64-windows
```

---

## 🔧 CMake 配置

如果使用 CMake 构建项目：

```cmake
cmake_minimum_required(VERSION 3.15)
project(LicenseSystem)

set(CMAKE_CXX_STANDARD 17)

# 查找 OpenSSL
find_package(OpenSSL REQUIRED)

# 查找 CURL
find_package(CURL REQUIRED)

# 添加可执行文件
add_executable(license_client
    http_client_cpp.cpp
    secure_transport_cpp.cpp
    win_product.cpp
    main.cpp
)

# 链接库
target_link_libraries(license_client
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
)

# 包含目录
target_include_directories(license_client PRIVATE
    ${OPENSSL_INCLUDE_DIR}
    ${CURL_INCLUDE_DIR}
)
```

---

## 📦 完整依赖对比表

| 功能模块      | Qt 版本                | 纯 C++ 版本                  | 说明            |
| ------------- | ---------------------- | ---------------------------- | --------------- |
| **网络通信**  | QNetworkAccessManager  | **libcurl**                  | HTTP/HTTPS 请求 |
| **JSON 解析** | QJsonDocument          | **nlohmann/json** 或简化实现 | JSON 处理       |
| **加密哈希**  | QCryptographicHash     | **OpenSSL**                  | SHA256/HMAC     |
| **Base64**    | QByteArray::toBase64() | **OpenSSL BIO**              | 编码/解码       |
| **随机数**    | QRandomGenerator       | **OpenSSL RAND**             | 安全随机数      |
| **时间戳**    | QDateTime              | **std::time()**              | C++ 标准库      |

---

## 💡 使用示例

### 纯 C++ 版本

```cpp
#include "http_client_cpp.h"
#include "secure_transport_cpp.h"
#include "win_product.h"

int main()
{
    // 1. 设置应用密钥
    SecureTransportCpp::setAppSecret("YOUR_SECRET_2026");
    
    // 2. 创建 HTTP 客户端
    LicenseClientCpp client("https://yourserver.com/api");
    client.setAppSecret("YOUR_SECRET_2026");
    
    // 3. 获取机器码
    std::string machineCode = GenerateMachineCode();
    
    // 4. 请求授权
    auto response = client.requestLicense(machineCode, "user@example.com");
    
    if (response.success)
    {
        std::cout << "License Key: " << response.licenseKey << std::endl;
        
        // 保存到文件...
    }
    else
    {
        std::cerr << "Error: " << response.message << std::endl;
    }
    
    return 0;
}
```

### 集成到 Qt UI 项目

```cpp
// main.cpp - Qt 项目
#include <QApplication>
#include <QMainWindow>
#include <QMessageBox>
#include "http_client_cpp.h"  // 纯 C++ 网络模块
#include "win_product.h"       // 纯 C++ 机器码模块

class MainWindow : public QMainWindow
{
public:
    MainWindow()
    {
        // 启动时验证授权（使用纯 C++）
        checkLicense();
    }

private:
    void checkLicense()
    {
        // 纯 C++ 实现，不依赖 Qt 网络模块
        LicenseClientCpp client("https://yourserver.com/api");
        std::string machineCode = GenerateMachineCode();
        
        auto response = client.verifyLicense(machineCode, loadLicense());
        
        if (response.valid)
        {
            // Qt UI 显示成功
            QMessageBox::information(this, "授权成功", "软件已授权");
        }
        else
        {
            // Qt UI 显示失败
            QMessageBox::warning(this, "授权失败", 
                QString::fromStdString(response.message));
            QApplication::quit();
        }
    }
    
    std::string loadLicense()
    {
        // 从文件读取...
        return "";
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
```

---

## 🚀 快速开始（vcpkg）

### 1. 安装 vcpkg

```bash
# Windows
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 集成到 Visual Studio
.\vcpkg integrate install
```

### 2. 安装依赖

```bash
vcpkg install openssl:x64-windows
vcpkg install curl:x64-windows
vcpkg install nlohmann-json:x64-windows  # 可选
```

### 3. Visual Studio 项目配置

vcpkg 会自动集成，无需手动配置路径！

---

## 📝 库说明对比

### OpenSSL

**优点**:
- ✅ 行业标准，极其成熟
- ✅ 功能全面（加密、哈希、证书）
- ✅ 跨平台
- ✅ 高性能

**缺点**:
- ⚠️ 体积较大（约 5MB）
- ⚠️ API 较复杂

**替代方案**:
- **Crypto++**: 功能相似，纯 C++
- **mbedTLS**: 轻量级，适合嵌入式
- **Botan**: 现代 C++ 设计

### libcurl

**优点**:
- ✅ 最流行的 HTTP 库
- ✅ 支持 HTTPS、HTTP/2
- ✅ 跨平台
- ✅ API 简单

**缺点**:
- ⚠️ C 风格 API

**替代方案**:
- **cpp-httplib**: Header-Only，C++ 风格
- **Boost.Beast**: 异步，需要 Boost
- **POCO**: 完整的网络库

---

## 🎓 推荐方案

### 方案 A: 最小依赖（推荐）

```
OpenSSL + libcurl + 自己实现简单 JSON
```

**优点**: 依赖少，代码已完成  
**缺点**: JSON 解析功能有限

### 方案 B: 标准方案

```
OpenSSL + libcurl + nlohmann/json
```

**优点**: JSON 处理强大，Header-Only  
**缺点**: 编译时间稍长

### 方案 C: 纯 Header-Only（最简单）

```
OpenSSL + cpp-httplib + nlohmann/json
```

**优点**: 不需要链接额外的 .lib 文件  
**缺点**: cpp-httplib 功能比 libcurl 少

---

## 🔍 功能对比总结

| 特性     | Qt 版本         | 纯 C++ 版本            | 说明     |
| -------- | --------------- | ---------------------- | -------- |
| UI 框架  | ✅ 使用 Qt       | ✅ 使用 Qt              | 保持不变 |
| 网络通信 | QNetwork*       | libcurl                | 更轻量   |
| JSON     | QJson*          | nlohmann/json          | 更现代   |
| 加密     | QCrypto*        | OpenSSL                | 更标准   |
| 依赖大小 | Qt 全套 (~50MB) | OpenSSL + curl (~10MB) | 减少 80% |
| 编译速度 | 慢              | 快                     | 依赖少   |
| 跨平台   | 优秀            | 优秀                   | 都支持   |

---

## ✅ 结论

**推荐使用纯 C++ 版本**：
1. ✅ 只在 UI 部分使用 Qt
2. ✅ 核心功能用标准 C++ + OpenSSL + libcurl
3. ✅ 减少依赖，提高性能
4. ✅ 代码更容易移植到非 Qt 项目

已为你创建的文件：
- [secure_transport_cpp.h](computer_id/secure_transport_cpp.h) - 纯 C++ 加密模块
- [secure_transport_cpp.cpp](computer_id/secure_transport_cpp.cpp) - 实现
- [http_client_cpp.h](computer_id/http_client_cpp.h) - 纯 C++ HTTP 客户端
- [http_client_cpp.cpp](computer_id/http_client_cpp.cpp) - 实现

所有代码都是**纯 C++11 标准**，不依赖 Qt！
