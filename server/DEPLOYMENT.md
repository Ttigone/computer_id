# 服务端部署指南

## 概述

本指南介绍如何部署和运行许可证授权服务端，与 Qt Hybrid 客户端配合使用。

---

## 📋 目录

1. [快速开始（本地测试）](#快速开始本地测试)
2. [生产环境部署](#生产环境部署)
3. [配置说明](#配置说明)
4. [API 接口](#api-接口)
5. [安全配置](#安全配置)
6. [数据库管理](#数据库管理)
7. [故障排查](#故障排查)

---

## 快速开始（本地测试）

### 1. 安装依赖

```bash
# 进入服务端目录
cd server

# 安装 Python 依赖
pip install flask flask-cors

# 或使用 requirements.txt
pip install -r requirements.txt
```

### 2. 配置密钥

编辑 [secure_license_server.py](secure_license_server.py#L26-L28)：

```python
# 配置（必须与客户端保持一致）
APP_SECRET = "YOUR_STRONG_SECRET_2026"  # ⚠️ 改为强密码
SECRET_KEY = "YOUR_SECRET_KEY_2026"      # ⚠️ 改为强密码
DATABASE = "licenses.db"
```

⚠️ **重要**：客户端的 `setAppSecret()` 必须使用相同的 `APP_SECRET`！

### 3. 启动服务器

```bash
# 开发模式（带调试信息）
python secure_license_server.py

# 或直接运行
python server/secure_license_server.py
```

**输出示例：**
```
==================================================
Secure License Server Starting...
==================================================
Security Level: HIGH
Request Age Limit: 300s
Database: licenses.db
==================================================
 * Running on http://0.0.0.0:5000
 * Serving Flask app 'secure_license_server'
 * Debug mode: on
```

### 4. 测试服务器

在另一个终端测试：

```bash
# 健康检查
curl http://localhost:5000/api/health

# 预期响应
{
  "status": "ok",
  "timestamp": "2026-01-03T10:00:00.123456",
  "security": "enabled"
}
```

### 5. 配置客户端

在客户端 [license_main_window.cpp](../qt_hybrid/license_main_window.cpp#L15-L16) 中设置：

```cpp
// 构造函数中
LicenseMainWindow::LicenseMainWindow(QWidget* parent)
    : QMainWindow(parent) {
    
    // 配置服务端地址
    m_backend.setServerUrl("http://localhost:5000");  // 本地测试
    
    // 配置密钥（必须与服务端一致）
    m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");
    
    // ... 其他代码
}
```

### 6. 运行客户端

启动 Qt 应用程序，现在就可以：
- ✅ 获取机器码
- ✅ 申请许可证
- ✅ 验证许可证

---

## 生产环境部署

### 方案 1：使用 Gunicorn（推荐）

#### 安装

```bash
pip install gunicorn
```

#### 启动

```bash
# HTTP 模式（适合放在 Nginx 后面）
gunicorn -w 4 -b 0.0.0.0:5000 secure_license_server:app

# 参数说明：
# -w 4: 4 个工作进程
# -b 0.0.0.0:5000: 绑定到所有接口的 5000 端口
# secure_license_server:app: 模块名:应用对象
```

#### 后台运行

```bash
# 使用 nohup
nohup gunicorn -w 4 -b 0.0.0.0:5000 secure_license_server:app > server.log 2>&1 &

# 使用 systemd（推荐）
# 创建 /etc/systemd/system/license-server.service
```

#### systemd 服务配置

创建文件 `/etc/systemd/system/license-server.service`：

```ini
[Unit]
Description=License Server
After=network.target

[Service]
Type=notify
User=www-data
Group=www-data
WorkingDirectory=/path/to/computer_id/server
Environment="PATH=/usr/bin:/usr/local/bin"
ExecStart=/usr/local/bin/gunicorn \
    -w 4 \
    -b 127.0.0.1:5000 \
    --access-logfile /var/log/license-server/access.log \
    --error-logfile /var/log/license-server/error.log \
    secure_license_server:app

Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

启动服务：

```bash
sudo systemctl daemon-reload
sudo systemctl start license-server
sudo systemctl enable license-server  # 开机自启
sudo systemctl status license-server  # 查看状态
```

---

### 方案 2：使用 uWSGI

#### 安装

```bash
pip install uwsgi
```

#### 配置文件

创建 `uwsgi.ini`：

```ini
[uwsgi]
module = secure_license_server:app
master = true
processes = 4
socket = /tmp/license-server.sock
chmod-socket = 660
vacuum = true
die-on-term = true
```

#### 启动

```bash
uwsgi --ini uwsgi.ini
```

---

### 方案 3：使用 Docker（最简单）

#### Dockerfile

创建 `server/Dockerfile`：

```dockerfile
FROM python:3.11-slim

WORKDIR /app

# 安装依赖
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 复制代码
COPY secure_license_server.py .

# 暴露端口
EXPOSE 5000

# 启动服务
CMD ["gunicorn", "-w", "4", "-b", "0.0.0.0:5000", "secure_license_server:app"]
```

#### docker-compose.yml

```yaml
version: '3.8'

services:
  license-server:
    build: ./server
    ports:
      - "5000:5000"
    volumes:
      - ./server/licenses.db:/app/licenses.db
    environment:
      - APP_SECRET=YOUR_STRONG_SECRET_2026
      - SECRET_KEY=YOUR_SECRET_KEY_2026
    restart: unless-stopped
```

#### 运行

```bash
docker-compose up -d
```

---

### Nginx 反向代理（HTTPS）

#### 安装 Certbot（Let's Encrypt 免费证书）

```bash
# Ubuntu/Debian
sudo apt install certbot python3-certbot-nginx

# 获取证书
sudo certbot --nginx -d yourdomain.com
```

#### Nginx 配置

编辑 `/etc/nginx/sites-available/license-server`：

```nginx
server {
    listen 443 ssl http2;
    server_name yourdomain.com;

    # SSL 证书
    ssl_certificate /etc/letsencrypt/live/yourdomain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/yourdomain.com/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;

    # 日志
    access_log /var/log/nginx/license-server-access.log;
    error_log /var/log/nginx/license-server-error.log;

    # 代理到 Flask
    location /api/ {
        proxy_pass http://127.0.0.1:5000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # 健康检查
    location /api/health {
        proxy_pass http://127.0.0.1:5000/api/health;
    }
}

# HTTP 重定向到 HTTPS
server {
    listen 80;
    server_name yourdomain.com;
    return 301 https://$server_name$request_uri;
}
```

启用配置：

```bash
sudo ln -s /etc/nginx/sites-available/license-server /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

---

## 配置说明

### 环境变量（推荐）

不要在代码中硬编码密钥，使用环境变量：

```python
import os

APP_SECRET = os.environ.get('APP_SECRET', 'DEFAULT_APP_SECRET_2026_CHANGE_THIS')
SECRET_KEY = os.environ.get('SECRET_KEY', 'DEFAULT_SECRET_KEY_2026')
DATABASE = os.environ.get('DATABASE_PATH', 'licenses.db')
```

设置环境变量：

```bash
# Linux/Mac
export APP_SECRET="your_strong_secret_here"
export SECRET_KEY="your_secret_key_here"

# Windows
set APP_SECRET=your_strong_secret_here
set SECRET_KEY=your_secret_key_here

# 永久设置（添加到 ~/.bashrc 或 ~/.zshrc）
echo 'export APP_SECRET="your_strong_secret_here"' >> ~/.bashrc
```

### 配置文件

创建 `server/config.py`：

```python
import os

class Config:
    APP_SECRET = os.environ.get('APP_SECRET', 'DEFAULT_SECRET')
    SECRET_KEY = os.environ.get('SECRET_KEY', 'DEFAULT_KEY')
    DATABASE = os.environ.get('DATABASE_PATH', 'licenses.db')
    MAX_REQUEST_AGE = int(os.environ.get('MAX_REQUEST_AGE', 300))
    
    # 速率限制
    RATE_LIMIT_REQUESTS = int(os.environ.get('RATE_LIMIT_REQUESTS', 100))
    RATE_LIMIT_WINDOW = int(os.environ.get('RATE_LIMIT_WINDOW', 3600))
    
    # 许可证配置
    DEFAULT_LICENSE_DAYS = int(os.environ.get('LICENSE_DAYS', 365))

# 使用
from config import Config
APP_SECRET = Config.APP_SECRET
```

---

## API 接口

### 1. 健康检查

```http
GET /api/health
```

**响应：**
```json
{
  "status": "ok",
  "timestamp": "2026-01-03T10:00:00",
  "security": "enabled"
}
```

---

### 2. 申请许可证

```http
POST /api/license/request
Content-Type: application/json

{
  "machine_code": "abc123...",
  "timestamp": 1704268800,
  "nonce": "random_string",
  "signature": "hmac_sha256_signature",
  "user_info": "user@example.com"
}
```

**响应（成功）：**
```json
{
  "success": true,
  "license_key": "LICENSE-KEY-HERE",
  "expires_at": "2027-01-03",
  "message": "License granted"
}
```

---

### 3. 验证许可证

```http
POST /api/license/verify
Content-Type: application/json

{
  "machine_code": "abc123...",
  "license_key": "LICENSE-KEY-HERE",
  "timestamp": 1704268800,
  "nonce": "random_string",
  "signature": "hmac_sha256_signature"
}
```

**响应（有效）：**
```json
{
  "valid": true,
  "message": "License is valid",
  "expires_at": "2027-01-03"
}
```

---

### 4. 查询许可证信息

```http
GET /api/license/info?machine_code=abc123...&timestamp=1704268800&nonce=xyz&signature=sig
```

**响应：**
```json
{
  "success": true,
  "license_info": {
    "status": "active",
    "user_info": "user@example.com",
    "created_at": "2026-01-03",
    "expires_at": "2027-01-03",
    "last_verified": "2026-01-03 10:00:00"
  }
}
```

---

## 安全配置

### 1. 更改默认密钥 ⚠️

```python
# ❌ 不安全
APP_SECRET = "DEFAULT_APP_SECRET_2026_CHANGE_THIS"

# ✅ 安全
APP_SECRET = "xK9$mP2#vL8@nQ5&wR4*zT7!yU6^aS3"
```

生成强密码：

```bash
# Linux/Mac
openssl rand -base64 32

# Python
python -c "import secrets; print(secrets.token_urlsafe(32))"
```

### 2. 启用 HTTPS

生产环境**必须**使用 HTTPS！

```cpp
// 客户端配置
m_backend.setServerUrl("https://yourdomain.com");  // ✅ HTTPS
```

### 3. 配置防火墙

```bash
# Ubuntu UFW
sudo ufw allow 443/tcp
sudo ufw allow 80/tcp
sudo ufw enable

# CentOS firewalld
sudo firewall-cmd --permanent --add-service=http
sudo firewall-cmd --permanent --add-service=https
sudo firewall-cmd --reload
```

### 4. IP 白名单（可选）

在 `secure_license_server.py` 中添加：

```python
ALLOWED_IPS = ['192.168.1.100', '10.0.0.50']

@app.before_request
def check_ip():
    if request.remote_addr not in ALLOWED_IPS:
        return jsonify({'error': 'Access denied'}), 403
```

---

## 数据库管理

### 查看许可证

```bash
# 安装 sqlite3
sudo apt install sqlite3  # Linux
# Windows: 从 https://sqlite.org/download.html 下载

# 连接数据库
cd server
sqlite3 licenses.db

# 查看所有许可证
SELECT * FROM licenses;

# 查看特定机器码
SELECT * FROM licenses WHERE machine_code = 'YOUR_MACHINE_CODE';

# 查看即将过期的许可证
SELECT * FROM licenses WHERE expires_at < date('now', '+30 days');
```

### 手动添加许可证

```sql
INSERT INTO licenses (
    machine_code,
    license_key,
    user_info,
    status,
    created_at,
    expires_at
) VALUES (
    'your_machine_code_here',
    'LICENSE-KEY-' || hex(randomblob(16)),
    'admin@example.com',
    'active',
    datetime('now'),
    datetime('now', '+365 days')
);
```

### 数据库备份

```bash
# 备份
sqlite3 licenses.db ".backup licenses_backup.db"

# 或使用 cp
cp licenses.db licenses_backup_$(date +%Y%m%d).db

# 定时备份（crontab）
0 2 * * * /usr/bin/sqlite3 /path/to/licenses.db ".backup /path/to/backup/licenses_$(date +\%Y\%m\%d).db"
```

---

## 故障排查

### 问题 1：客户端无法连接服务器

**检查：**
```bash
# 测试服务器是否运行
curl http://localhost:5000/api/health

# 测试远程连接
curl http://your-server-ip:5000/api/health

# 检查防火墙
sudo ufw status
sudo netstat -tlnp | grep 5000
```

**解决：**
- 确保服务器正在运行
- 检查防火墙规则
- 验证 IP 地址和端口

---

### 问题 2：签名验证失败

**错误：** `Invalid signature`

**原因：** 客户端和服务端的 `APP_SECRET` 不一致

**解决：**
```cpp
// 客户端
m_backend.setAppSecret("YOUR_STRONG_SECRET_2026");
```

```python
# 服务端
APP_SECRET = "YOUR_STRONG_SECRET_2026"
```

---

### 问题 3：请求过期

**错误：** `Request expired`

**原因：** 客户端和服务器时间相差超过 5 分钟

**解决：**
```bash
# 同步时间（Linux）
sudo ntpdate pool.ntp.org
sudo systemctl start systemd-timesyncd

# Windows
w32tm /resync
```

---

### 问题 4：数据库锁定

**错误：** `database is locked`

**原因：** 多个进程同时访问数据库

**解决：**
- 使用 Gunicorn 单进程模式测试：`gunicorn -w 1 ...`
- 考虑迁移到 PostgreSQL 或 MySQL

---

### 问题 5：速率限制触发

**错误：** `Too many requests`

**解决：**
```python
# 调整限制
@rate_limit(max_requests=200, window=3600)  # 每小时 200 次
```

---

## 监控和日志

### 查看日志

```bash
# Gunicorn 日志
tail -f /var/log/license-server/error.log
tail -f /var/log/license-server/access.log

# systemd 日志
sudo journalctl -u license-server -f

# Nginx 日志
tail -f /var/log/nginx/license-server-access.log
```

### 监控工具

```bash
# 安装 htop
sudo apt install htop

# 查看进程
ps aux | grep gunicorn

# 查看端口
sudo netstat -tlnp | grep 5000
```

---

## 生产环境检查清单

- [ ] 更改默认密钥（`APP_SECRET`, `SECRET_KEY`）
- [ ] 启用 HTTPS（Let's Encrypt）
- [ ] 配置 Nginx 反向代理
- [ ] 使用 Gunicorn 或 uWSGI
- [ ] 配置 systemd 服务（开机自启）
- [ ] 设置数据库备份（定时任务）
- [ ] 配置防火墙
- [ ] 配置日志轮转
- [ ] 设置监控和告警
- [ ] 测试所有 API 接口

---

## 相关文档

- [客户端配置](../qt_hybrid/README.md)
- [安全指南](../SECURITY_GUIDE.md)
- [API 测试](test_api.py)

---

## 技术支持

遇到问题？

1. 查看服务器日志
2. 测试 API 接口
3. 检查配置文件
4. 参考故障排查章节
