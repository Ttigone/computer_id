# Qt UI + 纯 C++ 后端 - 许可证管理系统

## 项目概述

这是一个混合架构的 Windows 许可证授权系统：
- **Qt 框架**：仅用于 UI 界面（QMainWindow、按钮、文本框等）
- **纯 C++**：所有业务逻辑（机器码生成、加密、网络通信）

## 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                   Qt UI Layer                           │
│  (license_main_window.h/cpp - QMainWindow)              │
│  ├─ 机器码显示区域                                       │
│  ├─ 许可证申请按钮                                       │
│  ├─ 许可证验证按钮                                       │
│  └─ 状态日志显示                                         │
└────────────────┬────────────────────────────────────────┘
                 │ 调用
                 ↓
┌─────────────────────────────────────────────────────────┐
│              Backend Wrapper Layer                       │
│  (license_backend.h/cpp - 纯 C++ 类)                    │
│  ├─ getMachineCode()                                     │
│  ├─ requestLicense()                                     │
│  ├─ verifyLicense()                                      │
│  └─ getLicenseInfo()                                     │
└────────────────┬────────────────────────────────────────┘
                 │ 调用
                 ↓
┌─────────────────────────────────────────────────────────┐
│            Pure C++ Business Logic                       │
│  ├─ win_product.cpp - WMI 机器码生成                    │
│  ├─ secure_transport_cpp.cpp - OpenSSL 加密             │
│  └─ http_client_cpp.cpp - libcurl 网络通信             │
└─────────────────────────────────────────────────────────┘
```

## 目录结构

```
computer_id/
├── qt_hybrid/                  # Qt UI + C++ 混合项目
│   ├── main.cpp                # Qt 应用程序入口
│   ├── license_main_window.h   # Qt 主窗口（UI 层）
│   ├── license_main_window.cpp
│   ├── license_backend.h       # 后端封装类（纯 C++）
│   ├── license_backend.cpp
│   ├── license_manager.pro     # qmake 项目文件
│   └── CMakeLists.txt          # CMake 配置文件
│
├── computer_id/                # 纯 C++ 模块（不依赖 Qt）
│   ├── win_product.h           # Windows 机器码生成
│   ├── win_product.cpp
│   ├── secure_transport_cpp.h  # OpenSSL 加密封装
│   ├── secure_transport_cpp.cpp
│   ├── http_client_cpp.h       # libcurl HTTP 客户端
│   └── http_client_cpp.cpp
│
└── server/                     # Python Flask 服务端
    └── secure_license_server.py
```

## 依赖库

### 1. Qt 框架（仅 UI）
- **Qt Core**：基础类型和事件循环
- **Qt Widgets**：窗口、按钮、文本框等 UI 控件

### 2. OpenSSL（加密）
- SHA256 哈希
- HMAC-SHA256 签名
- AES-256-CBC 加密
- Base64 编码

### 3. libcurl（网络）
- HTTPS 请求
- TLS/SSL 连接
- JSON 数据传输

### 4. Windows 系统库
- `wbemuuid.lib` - WMI 查询
- `ole32.lib` - COM 初始化
- `advapi32.lib` - 加密 API
- `ws2_32.lib` - Socket

## 安装依赖

### 方法 1: 使用 vcpkg（推荐）

```bash
# 安装 vcpkg（如果还没有）
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

# 安装依赖（x64 Windows）
vcpkg install openssl:x64-windows
vcpkg install curl:x64-windows

# 集成到 Visual Studio
vcpkg integrate install

# 设置环境变量（可选，用于 qmake）
setx VCPKG_ROOT "C:\vcpkg"
```

### 方法 2: 手动下载
- OpenSSL: https://slproweb.com/products/Win32OpenSSL.html
- libcurl: https://curl.se/windows/

## 编译项目

### 使用 CMake + Qt Creator

1. 打开 Qt Creator
2. File → Open File or Project → 选择 `CMakeLists.txt`
3. 配置构建套件（Kit）
4. Build → Build Project

### 使用 qmake

```bash
cd qt_hybrid
qmake license_manager.pro
nmake  # 或 mingw32-make
```

### 使用 CMake 命令行

```bash
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.0/msvc2019_64"
cmake --build . --config Release
```

## 运行项目

### 1. 启动服务端

```bash
cd server
pip install flask flask-cors
python secure_license_server.py
```

服务端默认运行在 `http://localhost:5000`

### 2. 配置客户端

修改 [license_main_window.cpp](qt_hybrid/license_main_window.cpp#L15-L16) 中的配置：

```cpp
// 服务端地址
m_backend.setServerUrl("http://localhost:5000");

// 应用密钥（必须与服务端一致）
m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");
```

### 3. 运行客户端

```bash
cd build
./LicenseManager  # 或 Release/LicenseManager.exe
```

## 使用流程

### 1. 获取机器码
- 点击"获取机器码"按钮
- 系统将读取 CPU、主板、硬盘序列号
- 生成 SHA256 哈希作为唯一机器码
- 可点击"复制"按钮复制机器码

### 2. 申请许可证
- 输入用户信息（可选）
- 点击"申请许可证"按钮
- 系统将机器码发送到服务端
- 服务端生成许可证密钥并返回
- 客户端自动保存到 `license.dat`

### 3. 验证许可证
- 点击"验证许可证"按钮
- 系统读取本地 `license.dat`
- 向服务端验证许可证有效性
- 显示许可证状态和到期时间

### 4. 查看许可证信息
- 点击"查看许可证"按钮
- 显示许可证创建时间、到期时间、用户信息

## 核心类说明

### LicenseMainWindow（Qt UI 层）

```cpp
class LicenseMainWindow : public QMainWindow
{
    Q_OBJECT

private slots:
    void onGetMachineCode();     // 获取机器码
    void onRequestLicense();     // 申请许可证
    void onVerifyLicense();      // 验证许可证
    void onViewLicense();        // 查看许可证信息

private:
    LicenseBackend m_backend;    // 纯 C++ 后端
    // UI 控件...
};
```

### LicenseBackend（纯 C++ 后端）

```cpp
class LicenseBackend
{
public:
    std::string getMachineCode();                    // 获取机器码
    void setServerUrl(const std::string& url);       // 设置服务端
    void setAppSecret(const std::string& secret);    // 设置密钥
    
    LicenseResponse requestLicense(...);             // 申请许可证
    VerifyResponse verifyLicense(...);               // 验证许可证
    LicenseInfo getLicenseInfo(...);                 // 查询信息
    
    static bool saveLicenseToFile(...);              // 保存许可证
    static std::string loadLicenseFromFile(...);     // 加载许可证

private:
    LicenseClientCpp* m_client;  // libcurl 客户端
};
```

## 异步执行模式

使用 `QThread::create()` 在后台线程执行 C++ 业务逻辑，避免阻塞 UI：

```cpp
void LicenseMainWindow::onRequestLicense()
{
    // 显示进度条
    m_progressBar->setVisible(true);
    
    // 在新线程执行
    QThread* thread = QThread::create([this, machineCode, userInfo]() {
        LicenseBackend backend;
        backend.setServerUrl(m_backend.getServerUrl());
        auto result = backend.requestLicense(machineCode, userInfo);
        
        // 切换回 UI 线程更新界面
        QMetaObject::invokeMethod(this, [this, result]() {
            if (result.success) {
                // 显示成功消息
                m_licenseKeyEdit->setText(QString::fromStdString(result.licenseKey));
            }
            m_progressBar->setVisible(false);
        }, Qt::QueuedConnection);
    });
    
    thread->start();
}
```

## 安全特性

### 1. 传输层安全
- **HTTPS/TLS 1.2+**：所有数据通过 SSL/TLS 加密传输
- **证书验证**：验证服务端证书有效性

### 2. 应用层安全
- **HMAC-SHA256 签名**：防止数据篡改
- **时间戳验证**：防止重放攻击（5 分钟有效期）
- **随机 Nonce**：每次请求唯一标识

### 3. 服务端安全
- **速率限制**：防止暴力破解（100 次/小时）
- **IP 记录**：审计和追踪
- **数据库加密**：敏感信息加密存储

## 部署说明

### 开发环境
1. 安装 Qt 5.15+ 或 Qt 6.x
2. 通过 vcpkg 安装 OpenSSL 和 libcurl
3. 编译运行即可

### 生产环境

#### 打包 Qt 应用

```bash
# 编译 Release 版本
cmake --build . --config Release

# 使用 windeployqt 复制依赖
cd Release
windeployqt LicenseManager.exe

# 手动复制 OpenSSL 和 libcurl DLL
copy C:\vcpkg\installed\x64-windows\bin\*.dll .
```

#### 部署服务端

```bash
# 使用生产级 WSGI 服务器
pip install gunicorn
gunicorn -w 4 -b 0.0.0.0:443 --certfile=cert.pem --keyfile=key.pem secure_license_server:app
```

或使用 Nginx 反向代理 + uWSGI。

## 常见问题

### 1. 编译错误：找不到 OpenSSL
**解决方案**：
```bash
vcpkg install openssl:x64-windows
vcpkg integrate install
```

### 2. 运行错误：libcurl.dll 缺失
**解决方案**：
```bash
copy C:\vcpkg\installed\x64-windows\bin\libcurl.dll Release\
```

### 3. WMI 查询失败
**解决方案**：
- 确保以管理员权限运行
- 检查 WMI 服务是否启动：`services.msc` → Windows Management Instrumentation

### 4. 网络连接失败
**解决方案**：
- 检查防火墙设置
- 确认服务端 URL 正确
- 测试网络连通性：`curl http://localhost:5000/api/license/info?machine_code=test`

### 5. HTTPS 证书错误
**解决方案**：
```cpp
// 在 http_client_cpp.cpp 中设置（仅开发环境）
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);  // 跳过证书验证
```

## 项目优势

1. ✅ **轻量级 Qt 依赖**：仅使用 QtCore 和 QtWidgets，不使用 QtNetwork
2. ✅ **跨平台 C++ 核心**：业务逻辑可移植到其他 GUI 框架（MFC、WinForms、GTK）
3. ✅ **标准库实现**：使用 OpenSSL 和 libcurl 而非专有库
4. ✅ **异步非阻塞**：UI 响应流畅，操作在后台线程执行
5. ✅ **安全可靠**：多层加密保护，防止破解和篡改
6. ✅ **易于扩展**：清晰的分层架构，易于添加新功能

## 许可证

本项目代码仅供学习和参考，请勿用于非法目的。

## 技术支持

如有问题，请参考以下文档：
- [安全指南](../SECURITY_GUIDE.md)
- [C++ 库使用](../CPP_LIBRARIES.md)
- [HTTPS 配置](../HTTPS_GUIDE.md)
