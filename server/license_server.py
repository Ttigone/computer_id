"""
许可证授权服务端 API
使用 Flask + SQLite 实现简单的授权服务器

功能:
1. 接收客户端机器码请求
2. 生成许可证并保存到数据库
3. 验证许可证有效性
4. 查询许可证信息

安装依赖:
pip install flask flask-cors hashlib
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import hashlib
import sqlite3
import json
from datetime import datetime, timedelta
import os

app = Flask(__name__)
CORS(app)  # 允许跨域请求

# 配置
SECRET_KEY = "DEFAULT_SECRET_KEY_2026"  # 应该与客户端保持一致
DATABASE = "licenses.db"

# ============================================================================
# 数据库初始化
# ============================================================================

def init_database():
    """初始化数据库表"""
    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()
    
    # 创建许可证表
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS licenses (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            machine_code TEXT UNIQUE NOT NULL,
            license_key TEXT NOT NULL,
            user_info TEXT,
            status TEXT DEFAULT 'active',
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            expires_at TIMESTAMP,
            last_verified TIMESTAMP
        )
    ''')
    
    # 创建验证日志表
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS verify_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            machine_code TEXT NOT NULL,
            verified_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            result TEXT,
            ip_address TEXT
        )
    ''')
    
    conn.commit()
    conn.close()
    print("Database initialized successfully")

# ============================================================================
# 工具函数
# ============================================================================

def generate_license_key(machine_code):
    """生成许可证密钥（与客户端逻辑一致）"""
    data = machine_code + SECRET_KEY
    return hashlib.sha256(data.encode()).hexdigest()

def get_db_connection():
    """获取数据库连接"""
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row  # 支持通过列名访问
    return conn

# ============================================================================
# API 路由
# ============================================================================

@app.route('/api/license/request', methods=['POST'])
def request_license():
    """
    处理许可证请求
    
    请求格式:
    {
        "machine_code": "abc123...",
        "user_info": "user@example.com"  (可选)
    }
    
    响应格式:
    {
        "success": true,
        "license_key": "def456...",
        "message": "License generated successfully",
        "expires_at": "2027-01-01 00:00:00"
    }
    """
    try:
        data = request.get_json()
        machine_code = data.get('machine_code', '').strip()
        user_info = data.get('user_info', '')
        
        if not machine_code:
            return jsonify({
                'success': False,
                'message': 'Machine code is required'
            }), 400
        
        # 生成许可证密钥
        license_key = generate_license_key(machine_code)
        
        # 设置过期时间（1年后）
        expires_at = datetime.now() + timedelta(days=365)
        
        # 保存到数据库
        conn = get_db_connection()
        cursor = conn.cursor()
        
        try:
            # 检查是否已存在
            existing = cursor.execute(
                'SELECT * FROM licenses WHERE machine_code = ?',
                (machine_code,)
            ).fetchone()
            
            if existing:
                # 更新现有记录
                cursor.execute('''
                    UPDATE licenses 
                    SET license_key = ?, user_info = ?, expires_at = ?, status = 'active'
                    WHERE machine_code = ?
                ''', (license_key, user_info, expires_at, machine_code))
                message = 'License renewed successfully'
            else:
                # 插入新记录
                cursor.execute('''
                    INSERT INTO licenses (machine_code, license_key, user_info, expires_at)
                    VALUES (?, ?, ?, ?)
                ''', (machine_code, license_key, user_info, expires_at))
                message = 'License generated successfully'
            
            conn.commit()
            
            # 记录日志
            print(f"[LICENSE ISSUED] Machine: {machine_code[:16]}... User: {user_info}")
            
            return jsonify({
                'success': True,
                'license_key': license_key,
                'message': message,
                'expires_at': expires_at.strftime('%Y-%m-%d %H:%M:%S')
            })
            
        finally:
            conn.close()
            
    except Exception as e:
        print(f"[ERROR] {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Server error: {str(e)}'
        }), 500

@app.route('/api/license/verify', methods=['POST'])
def verify_license():
    """
    验证许可证
    
    请求格式:
    {
        "machine_code": "abc123...",
        "license_key": "def456..."
    }
    
    响应格式:
    {
        "valid": true,
        "message": "License is valid",
        "expires_at": "2027-01-01 00:00:00"
    }
    """
    try:
        data = request.get_json()
        machine_code = data.get('machine_code', '').strip()
        license_key = data.get('license_key', '').strip()
        
        if not machine_code or not license_key:
            return jsonify({
                'valid': False,
                'message': 'Machine code and license key are required'
            }), 400
        
        # 从数据库查询
        conn = get_db_connection()
        cursor = conn.cursor()
        
        license_record = cursor.execute(
            'SELECT * FROM licenses WHERE machine_code = ? AND license_key = ?',
            (machine_code, license_key)
        ).fetchone()
        
        # 记录验证日志
        client_ip = request.remote_addr
        
        if license_record is None:
            result = 'not_found'
            cursor.execute(
                'INSERT INTO verify_logs (machine_code, result, ip_address) VALUES (?, ?, ?)',
                (machine_code, result, client_ip)
            )
            conn.commit()
            conn.close()
            
            return jsonify({
                'valid': False,
                'message': 'License not found or invalid'
            })
        
        # 检查状态
        if license_record['status'] != 'active':
            result = 'inactive'
            cursor.execute(
                'INSERT INTO verify_logs (machine_code, result, ip_address) VALUES (?, ?, ?)',
                (machine_code, result, client_ip)
            )
            conn.commit()
            conn.close()
            
            return jsonify({
                'valid': False,
                'message': f"License is {license_record['status']}"
            })
        
        # 检查过期时间
        if license_record['expires_at']:
            expires_at = datetime.strptime(license_record['expires_at'], '%Y-%m-%d %H:%M:%S.%f')
            if datetime.now() > expires_at:
                result = 'expired'
                cursor.execute(
                    'INSERT INTO verify_logs (machine_code, result, ip_address) VALUES (?, ?, ?)',
                    (machine_code, result, client_ip)
                )
                conn.commit()
                conn.close()
                
                return jsonify({
                    'valid': False,
                    'message': 'License has expired'
                })
        
        # 验证通过，更新最后验证时间
        result = 'success'
        cursor.execute(
            'UPDATE licenses SET last_verified = CURRENT_TIMESTAMP WHERE machine_code = ?',
            (machine_code,)
        )
        cursor.execute(
            'INSERT INTO verify_logs (machine_code, result, ip_address) VALUES (?, ?, ?)',
            (machine_code, result, client_ip)
        )
        conn.commit()
        conn.close()
        
        print(f"[LICENSE VERIFIED] Machine: {machine_code[:16]}... IP: {client_ip}")
        
        return jsonify({
            'valid': True,
            'message': 'License is valid',
            'expires_at': license_record['expires_at']
        })
        
    except Exception as e:
        print(f"[ERROR] {str(e)}")
        return jsonify({
            'valid': False,
            'message': f'Server error: {str(e)}'
        }), 500

@app.route('/api/license/info', methods=['POST'])
def get_license_info():
    """
    查询许可证信息
    
    请求格式:
    {
        "machine_code": "abc123..."
    }
    
    响应格式:
    {
        "success": true,
        "license_info": {
            "status": "active",
            "created_at": "2026-01-01 00:00:00",
            "expires_at": "2027-01-01 00:00:00",
            "last_verified": "2026-06-01 12:00:00"
        }
    }
    """
    try:
        data = request.get_json()
        machine_code = data.get('machine_code', '').strip()
        
        if not machine_code:
            return jsonify({
                'success': False,
                'message': 'Machine code is required'
            }), 400
        
        conn = get_db_connection()
        cursor = conn.cursor()
        
        license_record = cursor.execute(
            'SELECT * FROM licenses WHERE machine_code = ?',
            (machine_code,)
        ).fetchone()
        
        conn.close()
        
        if license_record is None:
            return jsonify({
                'success': False,
                'message': 'License not found'
            })
        
        return jsonify({
            'success': True,
            'license_info': {
                'status': license_record['status'],
                'user_info': license_record['user_info'],
                'created_at': license_record['created_at'],
                'expires_at': license_record['expires_at'],
                'last_verified': license_record['last_verified']
            }
        })
        
    except Exception as e:
        print(f"[ERROR] {str(e)}")
        return jsonify({
            'success': False,
            'message': f'Server error: {str(e)}'
        }), 500

@app.route('/api/license/revoke', methods=['POST'])
def revoke_license():
    """
    吊销许可证（管理员功能）
    
    请求格式:
    {
        "machine_code": "abc123...",
        "admin_key": "admin_secret"
    }
    """
    try:
        data = request.get_json()
        machine_code = data.get('machine_code', '').strip()
        admin_key = data.get('admin_key', '')
        
        # 简单的管理员验证（生产环境应使用更安全的方式）
        if admin_key != "admin_secret_2026":
            return jsonify({
                'success': False,
                'message': 'Unauthorized'
            }), 401
        
        conn = get_db_connection()
        cursor = conn.cursor()
        
        cursor.execute(
            'UPDATE licenses SET status = ? WHERE machine_code = ?',
            ('revoked', machine_code)
        )
        
        conn.commit()
        conn.close()
        
        print(f"[LICENSE REVOKED] Machine: {machine_code[:16]}...")
        
        return jsonify({
            'success': True,
            'message': 'License revoked successfully'
        })
        
    except Exception as e:
        return jsonify({
            'success': False,
            'message': f'Server error: {str(e)}'
        }), 500

@app.route('/api/health', methods=['GET'])
def health_check():
    """健康检查接口"""
    return jsonify({
        'status': 'ok',
        'timestamp': datetime.now().isoformat()
    })

# ============================================================================
# 启动服务器
# ============================================================================

if __name__ == '__main__':
    # 初始化数据库
    if not os.path.exists(DATABASE):
        print("Creating database...")
        init_database()
    
    # 启动 Flask 服务器
    print("=" * 50)
    print("License Server Starting...")
    print("=" * 50)
    print(f"Database: {DATABASE}")
    print(f"Server URL: http://localhost:5000")
    print("API Endpoints:")
    print("  - POST /api/license/request  (申请许可证)")
    print("  - POST /api/license/verify   (验证许可证)")
    print("  - POST /api/license/info     (查询信息)")
    print("  - POST /api/license/revoke   (吊销许可证)")
    print("  - GET  /api/health           (健康检查)")
    print("=" * 50)
    
    # 生产环境配置:
    # 1. 使用 HTTPS (需要 SSL 证书)
    # 2. 使用 gunicorn 或 uwsgi 运行
    # 3. 配置 nginx 反向代理
    # 4. 使用生产级数据库 (PostgreSQL/MySQL)
    
    # 开发环境
    app.run(host='0.0.0.0', port=5000, debug=True)
    
    # 生产环境示例 (使用 HTTPS):
    # app.run(host='0.0.0.0', port=5000, 
    #         ssl_context=('cert.pem', 'key.pem'))
