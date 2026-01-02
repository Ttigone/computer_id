#!/bin/bash
# 快速启动许可证服务器（开发模式）

echo "======================================"
echo "许可证服务器 - 快速启动脚本"
echo "======================================"
echo ""

# 检查 Python
if ! command -v python3 &> /dev/null; then
    echo "错误: 未找到 Python3！"
    echo "请先安装 Python3"
    exit 1
fi

echo "[1/3] 检查依赖..."
if ! python3 -c "import flask" 2>/dev/null; then
    echo "Flask 未安装，正在安装依赖..."
    pip3 install -r requirements.txt
else
    echo "依赖已安装 ✓"
fi

echo ""
echo "[2/3] 配置说明"
echo "============================================"
echo "⚠️  重要：请修改密钥配置！"
echo ""
echo "编辑 secure_license_server.py 第 26-28 行："
echo "  APP_SECRET = \"YOUR_STRONG_SECRET_2026\""
echo "  SECRET_KEY = \"YOUR_SECRET_KEY_2026\""
echo ""
echo "客户端也要使用相同的 APP_SECRET！"
echo "============================================"
echo ""

echo "[3/3] 启动服务器..."
echo ""
echo "服务器将运行在: http://localhost:5000"
echo "健康检查: http://localhost:5000/api/health"
echo ""
echo "按 Ctrl+C 停止服务器"
echo "============================================"
echo ""

# 启动服务器
python3 secure_license_server.py
