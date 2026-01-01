@echo off
REM Windows 服务器部署脚本

echo ========================================
echo   License Server Deployment (Windows)
echo ========================================
echo.

REM 1. 检查 Python
echo Checking Python installation...
python --version
if errorlevel 1 (
    echo Error: Python not found! Please install Python 3.8+
    exit /b 1
)

REM 2. 安装依赖
echo.
echo Installing dependencies...
pip install -r requirements.txt

REM 3. 初始化数据库
echo.
echo Initializing database...
python license_server.py
if errorlevel 1 (
    echo Error: Failed to initialize database
    exit /b 1
)

echo.
echo ========================================
echo   Deployment Complete!
echo ========================================
echo.
echo Next steps:
echo 1. Configure firewall to allow port 5000
echo 2. For HTTPS, obtain SSL certificate
echo 3. Run: python license_server.py
echo.
echo For production deployment, consider:
echo - Using waitress or gunicorn for WSGI
echo - Setting up nginx as reverse proxy
echo - Using a production database (PostgreSQL/MySQL)
echo.

pause
