FROM python:3.11-slim

# 设置工作目录
WORKDIR /app

# 安装依赖
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# 复制服务器代码
COPY secure_license_server.py .

# 创建数据目录
RUN mkdir -p /app/data

# 暴露端口
EXPOSE 5000

# 环境变量
ENV APP_SECRET="DEFAULT_APP_SECRET_2026_CHANGE_THIS"
ENV SECRET_KEY="DEFAULT_SECRET_KEY_2026"
ENV DATABASE="/app/data/licenses.db"
ENV PYTHONUNBUFFERED=1

# 健康检查
HEALTHCHECK --interval=30s --timeout=5s --start-period=5s --retries=3 \
  CMD python -c "import requests; requests.get('http://localhost:5000/api/health')"

# 使用 gunicorn 启动
CMD ["gunicorn", "-w", "4", "-b", "0.0.0.0:5000", "--access-logfile", "-", "--error-logfile", "-", "secure_license_server:app"]
