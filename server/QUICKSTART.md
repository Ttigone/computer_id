# 服务端快速入门指南

## 🚀 5 分钟快速启动

### Windows 用户

```cmd
cd server
start_server.bat
```

### Linux/Mac 用户

```bash
cd server
chmod +x start_server.sh
./start_server.sh
```

就这么简单！服务器将运行在 `http://localhost:5000`

---

## ⚙️ 配置客户端

在客户端代码中设置服务器地址：

```cpp
// qt_hybrid/license_main_window.cpp
LicenseMainWindow::LicenseMainWindow(QWidget* parent) {
    // 设置服务器地址
    m_backend.setServerUrl("http://localhost:5000");
    
    // 设置密钥（必须与服务端一致）
    m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");
}
```

⚠️ **重要**：密钥必须与服务端 `secure_license_server.py` 中的 `APP_SECRET` 完全一致！

---

## 🧪 测试服务器

### 方法 1：使用浏览器

打开浏览器访问：http://localhost:5000/api/health

应该看到：
```json
{
  "status": "ok",
  "timestamp": "2026-01-03T10:00:00",
  "security": "enabled"
}
```

### 方法 2：使用测试脚本

```bash
cd server
python test_server.py
```

### 方法 3：使用 curl

```bash
# 健康检查
curl http://localhost:5000/api/health

# 查看输出
# {"status":"ok","timestamp":"..."}
```

---

## 🔐 修改密钥（必须做！）

1. **编辑服务端配置**

打开 `server/secure_license_server.py`，修改第 26-28 行：

```python
# 配置（必须与客户端保持一致）
APP_SECRET = "YOUR_STRONG_SECRET_2026"  # ⚠️ 改成强密码
SECRET_KEY = "YOUR_SECRET_KEY_2026"      # ⚠️ 改成强密码
DATABASE = "licenses.db"
```

生成强密码：
```bash
# Linux/Mac
openssl rand -base64 32

# Windows PowerShell
[Convert]::ToBase64String((1..32 | ForEach-Object {Get-Random -Maximum 256}))
```

2. **编辑客户端配置**

打开 `qt_hybrid/license_main_window.cpp`，修改第 16 行：

```cpp
m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");  // 使用相同密钥
```

3. **重启服务器和客户端**

---

## 📦 使用流程

### 1. 启动服务器

```bash
cd server
python secure_license_server.py
```

### 2. 运行客户端

启动 Qt 应用程序：
```bash
cd qt_hybrid/out/build/x64-Debug
LicenseManager.exe
```

### 3. 获取机器码

在客户端点击 **"获取机器码"** 按钮

### 4. 申请许可证

- 输入用户信息（可选）
- 点击 **"在线申请授权"** 按钮
- 服务器自动生成并返回许可证

### 5. 验证许可证

点击 **"验证授权"** 按钮，查看许可证是否有效

---

## 📊 查看数据库

### 方法 1：使用 SQLite Browser

1. 下载安装：https://sqlitebrowser.org/
2. 打开 `server/licenses.db`
3. 查看 `licenses` 表

### 方法 2：使用命令行

```bash
cd server
sqlite3 licenses.db

# 查看所有许可证
SELECT * FROM licenses;

# 退出
.exit
```

---

## 🌐 部署到生产环境

### 方案 1：直接部署

```bash
# 安装 gunicorn
pip install gunicorn

# 启动（生产模式）
gunicorn -w 4 -b 0.0.0.0:5000 secure_license_server:app
```

### 方案 2：使用 Docker

```bash
cd server

# 构建镜像
docker build -t license-server .

# 运行容器
docker run -d -p 5000:5000 \
  -e APP_SECRET="YOUR_SECRET" \
  -v $(pwd)/data:/app/data \
  license-server
```

### 方案 3：使用 docker-compose

```bash
cd server

# 启动
docker-compose up -d

# 查看日志
docker-compose logs -f

# 停止
docker-compose down
```

---

## 🔧 常见问题

### Q1: 无法连接服务器

**解决：**
```bash
# 检查服务器是否运行
curl http://localhost:5000/api/health

# 检查防火墙
# Windows: 控制面板 -> Windows Defender 防火墙
# Linux: sudo ufw status
```

### Q2: 签名验证失败

**原因：** 客户端和服务端密钥不一致

**解决：** 确保 `APP_SECRET` 完全相同（区分大小写）

### Q3: 请求过期

**原因：** 系统时间不同步

**解决：**
```bash
# Windows
w32tm /resync

# Linux
sudo ntpdate pool.ntp.org
```

### Q4: 端口被占用

**解决：**
```bash
# 查看端口占用（Windows）
netstat -ano | findstr :5000

# 杀死进程
taskkill /PID <进程ID> /F

# 或修改端口
python secure_license_server.py  # 编辑最后一行的 port=5000
```

---

## 📚 更多文档

- **完整部署指南**: [DEPLOYMENT.md](DEPLOYMENT.md)
- **API 文档**: [DEPLOYMENT.md#api-接口](DEPLOYMENT.md#api-接口)
- **安全配置**: [DEPLOYMENT.md#安全配置](DEPLOYMENT.md#安全配置)
- **客户端文档**: [../qt_hybrid/README.md](../qt_hybrid/README.md)

---

## ✅ 检查清单

部署前确认：

- [ ] 已安装 Python 3.7+
- [ ] 已安装依赖（`pip install -r requirements.txt`）
- [ ] 已修改 `APP_SECRET` 和 `SECRET_KEY`
- [ ] 客户端密钥与服务端一致
- [ ] 服务器可以正常访问（`curl http://localhost:5000/api/health`）
- [ ] 防火墙允许端口 5000（生产环境用 443）

生产环境额外检查：

- [ ] 使用 HTTPS（不是 HTTP）
- [ ] 使用 Gunicorn 或 uWSGI（不是 Flask 开发服务器）
- [ ] 配置 Nginx 反向代理
- [ ] 设置数据库备份
- [ ] 配置日志和监控

---

## 🎉 完成！

现在你的许可证服务器已经运行了！

测试完整流程：
1. ✅ 服务器正常运行
2. ✅ 客户端可以连接
3. ✅ 成功获取机器码
4. ✅ 成功申请许可证
5. ✅ 成功验证许可证

有问题？查看：
- 服务器日志输出
- [DEPLOYMENT.md#故障排查](DEPLOYMENT.md#故障排查)
- 测试脚本：`python test_server.py`
