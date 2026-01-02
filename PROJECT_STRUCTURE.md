# 项目文件结构

```
computer_id/
│
├── 📁 qt_hybrid/                    # Qt UI + 纯 C++ 客户端（推荐使用）
│   ├── main.cpp                     # Qt 应用入口
│   ├── license_main_window.h/cpp    # Qt UI 界面
│   ├── license_backend.h/cpp        # 纯 C++ 业务逻辑封装
│   ├── CMakeLists.txt               # CMake 构建配置
│   ├── CMakeSettings.json           # Visual Studio CMake 配置
│   ├── copy_dlls.bat                # DLL 复制脚本
│   ├── README.md                    # 客户端使用文档
│   ├── CMAKE_CONFIG.md              # CMake 配置指南
│   ├── BUILD_FIXES.md               # 编译问题修复记录
│   └── POWERSHELL_FIX.md            # PowerShell 错误修复
│
├── 📁 computer_id/                  # 纯 C++ 核心模块（不依赖 Qt）
│   ├── win_product.h/cpp            # Windows WMI 机器码生成
│   ├── secure_transport_cpp.h/cpp   # OpenSSL 加密封装
│   ├── http_client_cpp.h/cpp        # libcurl HTTP 客户端
│   ├── computer_id.cpp              # 命令行工具（旧版）
│   └── computer_id.vcxproj          # Visual Studio 项目文件
│
├── 📁 server/                       # Python Flask 服务端 ⭐
│   ├── secure_license_server.py     # 主服务器程序（加密版）
│   ├── license_server.py            # 基础版服务器（已过时）
│   ├── test_server.py               # API 测试脚本
│   ├── test_api.py                  # API 测试脚本（旧版）
│   ├── requirements.txt             # Python 依赖
│   ├── start_server.bat             # Windows 快速启动脚本
│   ├── start_server.sh              # Linux/Mac 快速启动脚本
│   ├── Dockerfile                   # Docker 镜像配置
│   ├── docker-compose.yml           # Docker Compose 配置
│   ├── QUICKSTART.md                # 快速入门指南 ⭐⭐⭐
│   └── DEPLOYMENT.md                # 完整部署指南
│
├── 📁 docs/                         # 文档（如果有）
│
├── README.md                        # 项目主文档
├── SECURITY_GUIDE.md                # 安全指南
├── CPP_LIBRARIES.md                 # C++ 库使用说明
├── HTTPS_GUIDE.md                   # HTTPS 配置指南
└── computer_id.sln                  # Visual Studio 解决方案
```

---

## 快速导航

### 🎯 我想做什么？

#### 1️⃣ 我想快速测试整个系统

**步骤：**
```bash
# 1. 启动服务器
cd server
python secure_license_server.py

# 2. 编译并运行客户端
cd qt_hybrid
# 使用 Visual Studio 打开 CMakeLists.txt
# 编译并运行
```

**文档：**
- [服务端快速入门](server/QUICKSTART.md) ⭐
- [客户端 README](qt_hybrid/README.md)

---

#### 2️⃣ 我想部署到生产环境

**步骤：**
1. 修改密钥配置（服务端和客户端）
2. 配置 HTTPS（Let's Encrypt）
3. 使用 Gunicorn + Nginx
4. 设置数据库备份

**文档：**
- [完整部署指南](server/DEPLOYMENT.md)
- [安全配置指南](SECURITY_GUIDE.md)

---

#### 3️⃣ 我遇到了编译问题

**常见问题：**
- ❌ Qt 找不到
- ❌ OpenSSL 编译错误
- ❌ PowerShell 未找到
- ❌ DLL 缺失

**文档：**
- [CMake 配置问题](qt_hybrid/CMAKE_CONFIG.md)
- [编译错误修复](qt_hybrid/BUILD_FIXES.md)
- [PowerShell 问题](qt_hybrid/POWERSHELL_FIX.md)

---

#### 4️⃣ 我想了解技术细节

**文档：**
- [C++ 库使用说明](CPP_LIBRARIES.md)
- [安全机制详解](SECURITY_GUIDE.md)
- [HTTPS 配置](HTTPS_GUIDE.md)
- [API 文档](server/DEPLOYMENT.md#api-接口)

---

#### 5️⃣ 我想贡献代码或报告问题

**流程：**
1. Fork 本仓库
2. 创建功能分支
3. 提交 Pull Request

**技术栈：**
- 客户端：C++17, Qt6, OpenSSL 3.x, libcurl
- 服务端：Python 3.7+, Flask 3.0, SQLite
- 构建：CMake 3.16+, vcpkg

---

## 核心模块说明

### 客户端模块

#### Qt UI 层
- **文件**: `qt_hybrid/license_main_window.h/cpp`
- **功能**: 提供图形界面，处理用户交互
- **依赖**: Qt6 Core + Widgets

#### C++ 业务层
- **文件**: `qt_hybrid/license_backend.h/cpp`
- **功能**: 封装所有业务逻辑，不依赖 Qt
- **依赖**: 纯 C++ 核心模块

#### C++ 核心模块
- **win_product**: WMI 硬件查询，生成机器码
- **secure_transport_cpp**: OpenSSL 加密（HMAC, SHA256, AES）
- **http_client_cpp**: libcurl HTTP/HTTPS 客户端

---

### 服务端模块

#### API 服务
- **文件**: `server/secure_license_server.py`
- **功能**: 
  - 许可证申请 (`/api/license/request`)
  - 许可证验证 (`/api/license/verify`)
  - 许可证查询 (`/api/license/info`)
  - 健康检查 (`/api/health`)

#### 安全机制
- HMAC-SHA256 签名验证
- 时间戳防重放攻击
- 速率限制防暴力破解
- IP 记录和审计

#### 数据存储
- **数据库**: SQLite (`licenses.db`)
- **表结构**: 
  - machine_code (主键)
  - license_key
  - user_info
  - status (active/expired)
  - created_at, expires_at
  - last_verified

---

## 技术栈对比

### 方案 1: Qt Hybrid（当前使用）✅

| 组件     | 技术          | 优点             |
| -------- | ------------- | ---------------- |
| UI       | Qt6 Widgets   | 跨平台、成熟稳定 |
| 业务逻辑 | 纯 C++        | 可移植、高性能   |
| 加密     | OpenSSL 3.x   | 工业标准         |
| 网络     | libcurl       | 稳定可靠         |
| 构建     | CMake + vcpkg | 依赖管理方便     |

**适合场景**: 需要图形界面的 Windows 应用

---

### 方案 2: 命令行工具（已过时）

| 组件     | 技术   | 缺点       |
| -------- | ------ | ---------- |
| UI       | 命令行 | 用户体验差 |
| 业务逻辑 | C++    | 功能有限   |

**适合场景**: 自动化脚本、CI/CD 集成

---

## 开发建议

### 推荐的开发环境

**Windows:**
- Visual Studio 2022
- Qt 6.8.3
- vcpkg
- CMake 3.16+

**Linux:**
- GCC 9+ 或 Clang 10+
- Qt 6.x
- CMake 3.16+
- OpenSSL 3.x

---

### 推荐的开发流程

1. **本地开发**
   - 使用 Flask 开发服务器
   - HTTP 连接（localhost）
   - SQLite 数据库

2. **测试环境**
   - 使用 Gunicorn
   - HTTP 连接（内网）
   - SQLite 数据库

3. **生产环境**
   - Gunicorn + Nginx
   - HTTPS 连接（域名 + SSL 证书）
   - PostgreSQL/MySQL（可选）

---

## 安全注意事项

⚠️ **必须修改的配置**

1. **服务端密钥**
   ```python
   # server/secure_license_server.py
   APP_SECRET = "YOUR_STRONG_SECRET_2026"  # ⚠️ 改成强密码
   ```

2. **客户端密钥**
   ```cpp
   // qt_hybrid/license_main_window.cpp
   m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");  // 必须相同
   ```

3. **生产环境 HTTPS**
   ```cpp
   m_backend.setServerUrl("https://yourdomain.com");  // ✅ HTTPS
   ```

详细说明：[SECURITY_GUIDE.md](SECURITY_GUIDE.md)

---

## 许可证

本项目代码仅供学习和参考使用，请勿用于非法目的。

---

## 联系方式

如有问题或建议，请：
1. 查看相关文档
2. 提交 Issue
3. 发起 Pull Request
