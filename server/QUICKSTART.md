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

### 场景 1：本地测试（服务器和客户端在同一台机器）

```cpp
// qt_hybrid/license_main_window.cpp
LicenseMainWindow::LicenseMainWindow(QWidget* parent) {
    // 本地测试
    m_backend.setServerUrl("http://localhost:5000");
    
    // 设置密钥（必须与服务端一致）
    m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");
}
```

### 场景 2：局域网测试（服务器在另一台电脑）

```cpp
// 使用服务器的 IP 地址
m_backend.setServerUrl("http://192.168.1.100:5000");  // 替换为实际 IP
m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");
```

**如何查找服务器 IP：**
```bash
# Windows（在服务器上运行）
ipconfig

# Linux/Mac（在服务器上运行）
ip addr show
# 或
ifconfig
```

### 场景 3：生产环境（使用域名 + HTTPS）⭐ 推荐

```cpp
// 使用 HTTPS 和域名
m_backend.setServerUrl("https://license.yourdomain.com");  // 替换为你的域名
m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");
```

⚠️ **重要提醒：**
- ✅ 生产环境**必须**使用 HTTPS（不是 HTTP）
- ✅ 密钥必须与服务端 `secure_license_server.py` 中的 `APP_SECRET` 完全一致
- ✅ 确保防火墙允许端口访问（HTTP: 5000, HTTPS: 443）

---

## 🧪 测试服务器

### 方法 1：使用浏览器

#### 本地测试
打开浏览器访问：http://localhost:5000/api/health

#### 远程测试
访问：http://服务器IP:5000/api/health

例如：http://192.168.1.100:5000/api/health

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

**测试远程服务器：**
编辑 `test_server.py` 第 10 行：
```python
SERVER_URL = "http://192.168.1.100:5000"  # 改为服务器地址
```

### 方法 3：使用 curl

```bash
# 本地测试
curl http://localhost:5000/api/health

# 远程测试
curl http://192.168.1.100:5000/api/health

# HTTPS 测试
curl https://license.yourdomain.com/api/health
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

### 配置服务器监听地址

默认情况下，Flask 只监听 `localhost`，外部无法访问。需要修改为监听所有接口。

编辑 `secure_license_server.py` 最后一行：

```python
# ❌ 只能本机访问
app.run(host='127.0.0.1', port=5000, debug=True)

# ✅ 允许外部访问
app.run(host='0.0.0.0', port=5000, debug=False)
```

⚠️ **注意**：
- `host='0.0.0.0'` 表示监听所有网络接口
- `debug=False` 关闭调试模式（生产环境必须）
- 确保防火墙允许端口 5000

### 方案 1：直接部署（简单测试）

```bash
# 安装 gunicorn
pip install gunicorn

# 启动（监听所有接口）
gunicorn -w 4 -b 0.0.0.0:5000 secure_license_server:app

# 后台运行
nohup gunicorn -w 4 -b 0.0.0.0:5000 secure_license_server:app > server.log 2>&1 &
```

**客户端配置：**
```cpp
// 使用服务器的公网 IP 或内网 IP
m_backend.setServerUrl("http://123.45.67.89:5000");  // 公网 IP
// 或
m_backend.setServerUrl("http://192.168.1.100:5000"); // 内网 IP
```

### 方案 2：使用域名 + HTTPS（推荐）⭐

#### 步骤 1：准备域名

购买域名并添加 A 记录：
```
license.yourdomain.com  →  123.45.67.89（服务器 IP）
```

#### 步骤 2：安装 Nginx 和 Certbot

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install nginx certbot python3-certbot-nginx

# 启动 Nginx
sudo systemctl start nginx
```

#### 步骤 3：获取 SSL 证书（Let's Encrypt 免费）

```bash
# 自动配置 HTTPS
sudo certbot --nginx -d license.yourdomain.com

# 根据提示输入邮箱和同意条款
```

#### 步骤 4：配置 Nginx 反向代理

编辑 `/etc/nginx/sites-available/license-server`：

```nginx
server {
    listen 80;
    server_name license.yourdomain.com;
    return 301 https://$server_name$request_uri;
}

server {
    listen 443 ssl http2;
    server_name license.yourdomain.com;

    # SSL 证书（Certbot 自动配置）
    ssl_certificate /etc/letsencrypt/live/license.yourdomain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/license.yourdomain.com/privkey.pem;

    # 反向代理到 Flask
    location / {
        proxy_pass http://127.0.0.1:5000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

启用配置：
```bash
sudo ln -s /etc/nginx/sites-available/license-server /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

#### 步骤 5：启动 Flask 服务

```bash
# 使用 gunicorn（只监听本地，Nginx 反向代理）
gunicorn -w 4 -b 127.0.0.1:5000 secure_license_server:app
```

**客户端配置：**
```cpp
// 使用 HTTPS + 域名
m_backend.setServerUrl("https://license.yourdomain.com");
m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");
```

### 方案 3：使用 Docker

```bash
cd server

# 构建镜像
docker build -t license-server .

# 运行容器（映射到所有接口）
docker run -d -p 5000:5000 \
  -e APP_SECRET="YOUR_SECRET" \
  -v $(pwd)/data:/app/data \
  license-server
```

**客户端配置：**
```cpp
m_backend.setServerUrl("http://服务器IP:5000");
```

### 方案 4：使用 docker-compose（最简单）

编辑 `docker-compose.yml`：
```yaml
services:
  license-server:
    build: .
    ports:
      - "5000:5000"  # 映射到主机所有接口
    environment:
      - APP_SECRET=YOUR_STRONG_SECRET_2026
```

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

### 🔥 防火墙配置

#### Windows 服务器

```powershell
# 允许端口 5000
New-NetFirewallRule -DisplayName "License Server" -Direction Inbound -LocalPort 5000 -Protocol TCP -Action Allow

# HTTPS（如果使用 Nginx）
New-NetFirewallRule -DisplayName "HTTPS" -Direction Inbound -LocalPort 443 -Protocol TCP -Action Allow
```

#### Linux 服务器

```bash
# UFW（Ubuntu/Debian）
sudo ufw allow 5000/tcp
sudo ufw allow 443/tcp
sudo ufw enable

# firewalld（CentOS/RHEL）
sudo firewall-cmd --permanent --add-port=5000/tcp
sudo firewall-cmd --permanent --add-service=https
sudo firewall-cmd --reload
```

#### 云服务器（阿里云/腾讯云/AWS）

在控制台的**安全组规则**中添加：
- 入站规则：TCP 5000（或 443）
- 源地址：0.0.0.0/0（所有地址）或指定客户端 IP

---

### 📡 动态 IP 解决方案

如果服务器 IP 经常变化，可以使用：

#### 方案 1：DDNS（动态域名）

使用免费 DDNS 服务：
- **花生壳**: https://hsk.oray.com/
- **No-IP**: https://www.noip.com/
- **DuckDNS**: https://www.duckdns.org/

客户端配置：
```cpp
m_backend.setServerUrl("http://yourusername.ddns.net:5000");
```

#### 方案 2：内网穿透

使用工具将内网服务暴露到公网：
- **Ngrok**: https://ngrok.com/
- **frp**: https://github.com/fatedier/frp
- **花生壳内网穿透**

```bash
# 使用 Ngrok
ngrok http 5000

# 会生成一个公网地址，例如：
# https://abc123.ngrok.io
```

客户端配置：
```cpp
m_backend.setServerUrl("https://abc123.ngrok.io");
```

---

## 🔧 常见问题

### Q1: 无法连接服务器

**症状：** 客户端显示 "连接失败" 或 "无法访问服务器"

**排查步骤：**

1. **检查服务器是否运行**
```bash
# 在服务器上运行
curl http://localhost:5000/api/health
```

2. **检查服务器监听地址**
```bash
# 查看服务器进程
netstat -tlnp | grep 5000  # Linux
netstat -ano | findstr :5000  # Windows

# 应该显示：
# 0.0.0.0:5000  ✅ 允许外部访问
# 127.0.0.1:5000  ❌ 只能本机访问
```

3. **从客户端测试连接**
```bash
# 在客户端机器上运行
curl http://服务器IP:5000/api/health
ping 服务器IP
telnet 服务器IP 5000
```

4. **检查防火墙**
```bash
# Windows 服务器
Get-NetFirewallRule | Where-Object {$_.LocalPort -eq 5000}

# Linux 服务器
sudo ufw status
sudo firewall-cmd --list-ports
```

5. **检查云服务器安全组**
- 登录云服务商控制台
- 找到安全组规则
- 确认已开放端口 5000

**解决方案：**
```bash
# 修改 Flask 监听地址
# secure_license_server.py 最后一行改为：
app.run(host='0.0.0.0', port=5000, debug=False)

# 开放防火墙
sudo ufw allow 5000/tcp  # Linux
# 或在 Windows 防火墙中添加规则
```

### Q2: 签名验证失败

**原因：** 客户端和服务端密钥不一致

**解决：** 确保 `APP_SECRET` 完全相同（区分大小写）

**检查方法：**
```python
# 服务端
print(f"Server APP_SECRET: {APP_SECRET}")

# 客户端（添加调试输出）
std::cout << "Client APP_SECRET: " << app_secret << std::endl;
```

### Q3: 请求过期

**原因：** 系统时间不同步

**解决：**
```bash
# Windows
w32tm /resync

# Linux
sudo ntpdate pool.ntp.org
sudo systemctl restart systemd-timesyncd
```

### Q4: 端口被占用

**检查占用进程：**
```bash
# Windows
netstat -ano | findstr :5000
tasklist | findstr <PID>

# Linux
sudo lsof -i :5000
sudo netstat -tlnp | grep 5000
```

**解决：**
```bash
# 杀死占用进程
taskkill /PID <进程ID> /F  # Windows
sudo kill -9 <PID>  # Linux

# 或修改端口
python secure_license_server.py  # 编辑最后一行的 port=5000
```

### Q5: HTTPS 证书错误

**错误信息：** "SSL certificate verify failed"

**原因：** 使用自签名证书或证书过期

**临时解决（仅测试）：**
```cpp
// http_client_cpp.cpp 中添加（仅开发环境）
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
```

**正式解决：**
```bash
# 使用 Let's Encrypt 免费证书
sudo certbot --nginx -d license.yourdomain.com
```

### Q6: 跨域问题（仅浏览器客户端）

**错误：** "CORS policy: No 'Access-Control-Allow-Origin' header"

**解决：** 已在服务器中启用 CORS，如果仍有问题：
```python
# secure_license_server.py 开头
from flask_cors import CORS
CORS(app, resources={r"/api/*": {"origins": "*"}})
```

### Q7: 内网 IP 无法访问

**场景：** 客户端和服务器在不同网络

**解决方案：**

1. **使用公网 IP**（云服务器）
2. **使用 DDNS**（动态 IP）
3. **使用内网穿透**（Ngrok, frp）
4. **配置端口转发**（路由器）

### Q8: 数据库权限错误

**错误：** "OperationalError: unable to open database file"

**解决：**
```bash
# 检查数据库文件权限
ls -l licenses.db

# 修改权限
chmod 666 licenses.db
chmod 777 .  # 目录需要写入权限

# 或指定完整路径
DATABASE = "/var/lib/license-server/licenses.db"
```

---

## 📚 更多文档

- **完整部署指南**: [DEPLOYMENT.md](DEPLOYMENT.md)
- **API 文档**: [DEPLOYMENT.md#api-接口](DEPLOYMENT.md#api-接口)
- **安全配置**: [DEPLOYMENT.md#安全配置](DEPLOYMENT.md#安全配置)
- **客户端文档**: [../qt_hybrid/README.md](../qt_hybrid/README.md)

---

## 🌍 网络配置速查表

### 场景对照表

| 场景           | 服务器地址                 | 客户端配置                        | 注意事项     |
| -------------- | -------------------------- | --------------------------------- | ------------ |
| 本机测试       | `localhost:5000`           | `http://localhost:5000`           | 最简单       |
| 局域网测试     | `192.168.1.100:5000`       | `http://192.168.1.100:5000`       | 需开防火墙   |
| 公网 IP        | `123.45.67.89:5000`        | `http://123.45.67.89:5000`        | 建议用 HTTPS |
| 域名 + HTTP    | `license.example.com:5000` | `http://license.example.com:5000` | DNS 解析     |
| 域名 + HTTPS ⭐ | `license.example.com:443`  | `https://license.example.com`     | **推荐**     |
| 内网穿透       | `Ngrok/frp`                | `https://abc.ngrok.io`            | 临时方案     |

### 快速配置模板

#### 1. 本地开发（默认）

**服务器：**
```python
# secure_license_server.py
app.run(host='127.0.0.1', port=5000, debug=True)
```

**客户端：**
```cpp
m_backend.setServerUrl("http://localhost:5000");
```

---

#### 2. 局域网部署

**服务器：**
```python
# 监听所有接口
app.run(host='0.0.0.0', port=5000, debug=False)
```

**查找服务器 IP：**
```bash
# Windows
ipconfig
# 找到 IPv4 地址，如：192.168.1.100

# Linux
ip addr show
# 或
hostname -I
```

**客户端：**
```cpp
m_backend.setServerUrl("http://192.168.1.100:5000");
```

**防火墙：**
```bash
# Windows
netsh advfirewall firewall add rule name="License Server" dir=in action=allow protocol=TCP localport=5000

# Linux
sudo ufw allow 5000/tcp
```

---

#### 3. 公网部署（云服务器）⭐

**服务器配置：**
```bash
# 使用 Gunicorn
gunicorn -w 4 -b 0.0.0.0:5000 secure_license_server:app

# 或使用 systemd 服务
sudo systemctl start license-server
```

**云平台安全组：**
- 阿里云：ECS -> 安全组 -> 添加规则
- 腾讯云：CVM -> 安全组 -> 入站规则
- AWS：EC2 -> Security Groups -> Inbound rules

添加规则：
- 协议：TCP
- 端口：5000（或 443）
- 源：0.0.0.0/0（所有 IP）或指定 IP

**客户端：**
```cpp
// 使用公网 IP
m_backend.setServerUrl("http://123.45.67.89:5000");

// 或使用域名（推荐）
m_backend.setServerUrl("https://license.yourdomain.com");
```

---

#### 4. HTTPS 生产部署（最佳实践）⭐⭐⭐

**必需条件：**
- ✅ 域名（如 `license.example.com`）
- ✅ DNS 记录指向服务器 IP
- ✅ 80 和 443 端口开放

**快速配置：**
```bash
# 1. 安装 Nginx 和 Certbot
sudo apt install nginx certbot python3-certbot-nginx

# 2. 获取免费 SSL 证书
sudo certbot --nginx -d license.yourdomain.com

# 3. Certbot 会自动配置 Nginx

# 4. 启动 Flask（只监听本地）
gunicorn -w 4 -b 127.0.0.1:5000 secure_license_server:app

# 5. Nginx 自动转发 HTTPS 到 Flask
```

**客户端：**
```cpp
m_backend.setServerUrl("https://license.yourdomain.com");
```

**证书自动续期：**
```bash
# Certbot 会自动添加 cron 任务
sudo certbot renew --dry-run  # 测试续期
```

---

#### 5. 动态 IP 方案（家庭宽带）

**使用 DDNS：**

1. 注册免费 DDNS 服务（如花生壳）
2. 获取动态域名（如 `myserver.ddns.net`）
3. 安装 DDNS 客户端自动更新 IP

**客户端：**
```cpp
m_backend.setServerUrl("http://myserver.ddns.net:5000");
```

**路由器端口转发：**
- 外部端口：5000
- 内部 IP：192.168.1.100
- 内部端口：5000

---

#### 6. 内网穿透（临时测试）

**使用 Ngrok：**
```bash
# 安装 Ngrok
# 下载: https://ngrok.com/download

# 启动穿透
ngrok http 5000

# 会生成临时公网地址：
# https://abc123.ngrok.io
```

**客户端：**
```cpp
m_backend.setServerUrl("https://abc123.ngrok.io");
```

⚠️ 免费版地址会变，每次重启需要更新客户端配置。

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
