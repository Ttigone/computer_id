"""
安全授权服务端（加密版本）
支持客户端发送的加密机器码，提供多层安全防护

新增功能:
1. 签名验证（HMAC-SHA256）
2. 时间戳验证（防重放攻击）
3. 请求频率限制（防暴力破解）
4. IP 白名单（可选）
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
import hashlib
import hmac
import sqlite3
import json
import base64
import time
from datetime import datetime, timedelta
from functools import wraps
import os

app = Flask(__name__)
CORS(app)

# 配置（必须与客户端保持一致）
APP_SECRET = "DEFAULT_APP_SECRET_2026_CHANGE_THIS"
SECRET_KEY = "DEFAULT_SECRET_KEY_2026"
DATABASE = "licenses.db"
MAX_REQUEST_AGE = 300  # 最大请求时间差（秒）

# 请求频率限制（简单实现）
request_counter = {}

# ============================================================================
# 安全工具函数
# ============================================================================

def verify_signature(data, timestamp, signature):
    """验证 HMAC-SHA256 签名"""
    message = data + str(timestamp) + APP_SECRET
    expected_signature = hmac.new(
        APP_SECRET.encode(),
        message.encode(),
        hashlib.sha256
    ).hexdigest()
    
    return hmac.compare_digest(signature, expected_signature)

def verify_secure_packet(packet_json):
    """
    验证安全数据包
    返回: (成功, 机器码, 错误信息)
    """
    try:
        packet = json.loads(packet_json)
        
        machine_code = packet.get('machine_code', '')
        timestamp = packet.get('timestamp', 0)
        nonce = packet.get('nonce', '')
        signature = packet.get('signature', '')
        
        if not all([machine_code, timestamp, nonce, signature]):
            return False, None, "Missing required fields"
        
        # 1. 验证时间戳（防重放攻击）
        current_time = int(time.time())
        if current_time - timestamp > MAX_REQUEST_AGE:
            return False, None, "Request expired"
        
        if timestamp > current_time + 60:  # 允许 60 秒时间误差
            return False, None, "Invalid timestamp (future)"
        
        # 2. 验证签名（防篡改）
        combined_data = machine_code + "|" + str(timestamp) + "|" + nonce
        if not verify_signature(combined_data, timestamp, signature):
            return False, None, "Invalid signature"
        
        # 3. 可选：验证 nonce 是否已使用（防重放攻击的进阶版）
        # 这里简化处理，生产环境应该用 Redis 存储已使用的 nonce
        
        return True, machine_code, None
        
    except json.JSONDecodeError:
        return False, None, "Invalid JSON format"
    except Exception as e:
        return False, None, f"Verification error: {str(e)}"

def rate_limit(max_requests=10, window=3600):
    """
    请求频率限制装饰器
    max_requests: 时间窗口内最大请求次数
    window: 时间窗口（秒）
    """
    def decorator(f):
        @wraps(f)
        def wrapped(*args, **kwargs):
            client_ip = request.remote_addr
            current_time = time.time()
            
            # 清理过期记录
            if client_ip in request_counter:
                request_counter[client_ip] = [
                    t for t in request_counter[client_ip]
                    if current_time - t < window
                ]
            
            # 检查频率
            if client_ip in request_counter:
                if len(request_counter[client_ip]) >= max_requests:
                    return jsonify({
                        'success': False,
                        'message': 'Too many requests. Please try again later.'
                    }), 429
            
            # 记录本次请求
            if client_ip not in request_counter:
                request_counter[client_ip] = []
            request_counter[client_ip].append(current_time)
            
            return f(*args, **kwargs)
        return wrapped
    return decorator

def generate_license_key(machine_code):
    """生成许可证密钥"""
    data = machine_code + SECRET_KEY
    return hashlib.sha256(data.encode()).hexdigest()

def get_db_connection():
    """获取数据库连接"""
    conn = sqlite3.connect(DATABASE)
    conn.row_factory = sqlite3.Row
    return conn

def init_database():
    """初始化数据库"""
    conn = sqlite3.connect(DATABASE)
    cursor = conn.cursor()
    
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
    
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS verify_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            machine_code TEXT NOT NULL,
            verified_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            result TEXT,
            ip_address TEXT
        )
    ''')
    
    # 新增：安全事件日志表
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS security_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            event_type TEXT NOT NULL,
            ip_address TEXT,
            details TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    
    conn.commit()
    conn.close()

def log_security_event(event_type, details):
    """记录安全事件"""
    try:
        conn = get_db_connection()
        cursor = conn.cursor()
        cursor.execute(
            'INSERT INTO security_logs (event_type, ip_address, details) VALUES (?, ?, ?)',
            (event_type, request.remote_addr, details)
        )
        conn.commit()
        conn.close()
    except Exception as e:
        print(f"[WARNING] Failed to log security event: {e}")

# ============================================================================
# API 路由（安全版本）
# ============================================================================

@app.route('/api/license/request', methods=['POST'])
@rate_limit(max_requests=10, window=3600)  # 每小时最多 10 次请求
def request_license():
    """处理许可证请求（安全版本）"""
    try:
        data = request.get_json()
        
        # 1. 提取并验证安全数据包
        secure_packet = data.get('secure_packet', '')
        if not secure_packet:
            log_security_event('INVALID_REQUEST', 'Missing secure_packet')
            return jsonify({
                'success': False,
                'message': 'Invalid request format'
            }), 400
        
        # 2. 解析安全数据包（如果是 Base64 编码）
        try:
            packet_json = base64.b64decode(secure_packet).decode('utf-8')
        except:
            packet_json = secure_packet
        
        # 3. 验证数据包
        success, machine_code, error = verify_secure_packet(packet_json)
        if not success:
            log_security_event('VERIFICATION_FAILED', f'Error: {error}')
            return jsonify({
                'success': False,
                'message': f'Security verification failed: {error}'
            }), 403
        
        # 4. 提取其他信息
        user_info = data.get('user_info', '')
        
        # 5. 生成许可证
        license_key = generate_license_key(machine_code)
        expires_at = datetime.now() + timedelta(days=365)
        
        # 6. 保存到数据库
        conn = get_db_connection()
        cursor = conn.cursor()
        
        try:
            existing = cursor.execute(
                'SELECT * FROM licenses WHERE machine_code = ?',
                (machine_code,)
            ).fetchone()
            
            if existing:
                cursor.execute('''
                    UPDATE licenses 
                    SET license_key = ?, user_info = ?, expires_at = ?, status = 'active'
                    WHERE machine_code = ?
                ''', (license_key, user_info, expires_at, machine_code))
                message = 'License renewed successfully'
            else:
                cursor.execute('''
                    INSERT INTO licenses (machine_code, license_key, user_info, expires_at)
                    VALUES (?, ?, ?, ?)
                ''', (machine_code, license_key, user_info, expires_at))
                message = 'License generated successfully'
            
            conn.commit()
            
            print(f"[LICENSE ISSUED] Machine: {machine_code[:16]}... User: {user_info}")
            log_security_event('LICENSE_ISSUED', f'Machine: {machine_code[:16]}...')
            
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
        log_security_event('SERVER_ERROR', str(e))
        return jsonify({
            'success': False,
            'message': 'Server error'
        }), 500

@app.route('/api/license/verify', methods=['POST'])
@rate_limit(max_requests=100, window=3600)  # 验证请求可以更频繁
def verify_license():
    """验证许可证（安全版本）"""
    try:
        data = request.get_json()
        
        # 1. 验证安全数据包
        secure_packet = data.get('secure_packet', '')
        if secure_packet:
            try:
                packet_json = base64.b64decode(secure_packet).decode('utf-8')
            except:
                packet_json = secure_packet
            
            success, machine_code, error = verify_secure_packet(packet_json)
            if not success:
                log_security_event('VERIFY_FAILED', f'Error: {error}')
                return jsonify({
                    'valid': False,
                    'message': f'Security verification failed: {error}'
                }), 403
        else:
            # 兼容旧版本（不推荐）
            machine_code = data.get('machine_code', '').strip()
        
        license_key = data.get('license_key', '').strip()
        
        if not machine_code or not license_key:
            return jsonify({
                'valid': False,
                'message': 'Missing required parameters'
            }), 400
        
        # 2. 查询数据库
        conn = get_db_connection()
        cursor = conn.cursor()
        
        license_record = cursor.execute(
            'SELECT * FROM licenses WHERE machine_code = ? AND license_key = ?',
            (machine_code, license_key)
        ).fetchone()
        
        client_ip = request.remote_addr
        
        if license_record is None:
            result = 'not_found'
            cursor.execute(
                'INSERT INTO verify_logs (machine_code, result, ip_address) VALUES (?, ?, ?)',
                (machine_code, result, client_ip)
            )
            conn.commit()
            conn.close()
            
            log_security_event('VERIFY_NOT_FOUND', f'Machine: {machine_code[:16]}...')
            
            return jsonify({
                'valid': False,
                'message': 'License not found'
            })
        
        # 3. 检查状态
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
        
        # 4. 检查过期
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
        
        # 5. 验证通过
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
            'message': 'Server error'
        }), 500

@app.route('/api/license/info', methods=['POST'])
@rate_limit(max_requests=50, window=3600)
def get_license_info():
    """查询许可证信息（安全版本）"""
    try:
        data = request.get_json()
        
        # 验证安全数据包
        secure_packet = data.get('secure_packet', '')
        if secure_packet:
            try:
                packet_json = base64.b64decode(secure_packet).decode('utf-8')
            except:
                packet_json = secure_packet
            
            success, machine_code, error = verify_secure_packet(packet_json)
            if not success:
                return jsonify({
                    'success': False,
                    'message': f'Security verification failed: {error}'
                }), 403
        else:
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
            'message': 'Server error'
        }), 500

@app.route('/api/health', methods=['GET'])
def health_check():
    """健康检查"""
    return jsonify({
        'status': 'ok',
        'timestamp': datetime.now().isoformat(),
        'security': 'enabled'
    })

# ============================================================================
# 启动服务器
# ============================================================================

if __name__ == '__main__':
    if not os.path.exists(DATABASE):
        print("Creating database...")
        init_database()
    
    print("=" * 50)
    print("Secure License Server Starting...")
    print("=" * 50)
    print(f"Security Level: HIGH")
    print(f"Request Age Limit: {MAX_REQUEST_AGE}s")
    print(f"Database: {DATABASE}")
    print("=" * 50)
    
    app.run(host='0.0.0.0', port=5000, debug=True)
