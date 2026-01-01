#!/bin/bash
# Linux 服务器部署脚本

# 1. 安装 Python 依赖
echo "Installing Python dependencies..."
pip3 install -r requirements.txt

# 2. 初始化数据库
echo "Initializing database..."
python3 license_server.py &
sleep 3
kill %1

# 3. 生成 SSL 证书（自签名，仅用于测试）
echo "Generating SSL certificate..."
openssl req -x509 -newkey rsa:4096 -nodes \
    -out cert.pem -keyout key.pem -days 365 \
    -subj "/C=CN/ST=State/L=City/O=Organization/CN=localhost"

# 4. 使用 gunicorn 启动服务（生产环境）
echo "Starting license server with gunicorn..."
gunicorn -w 4 -b 0.0.0.0:5000 \
    --certfile=cert.pem --keyfile=key.pem \
    license_server:app

# 说明:
# -w 4: 使用 4 个 worker 进程
# -b 0.0.0.0:5000: 绑定到所有网络接口的 5000 端口
# --certfile/--keyfile: HTTPS 证书
