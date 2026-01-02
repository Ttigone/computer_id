"""
服务器 API 快速测试脚本
测试所有端点是否正常工作
"""

import requests
import json
import hashlib
import hmac
import time

# 配置
SERVER_URL = "http://localhost:5000"
APP_SECRET = "DEFAULT_APP_SECRET_2026_CHANGE_THIS"  # 与服务器保持一致

def generate_signature(machine_code, timestamp, nonce):
    """生成 HMAC-SHA256 签名"""
    combined_data = f"{machine_code}|{timestamp}|{nonce}"
    message = combined_data + str(timestamp) + APP_SECRET
    signature = hmac.new(
        APP_SECRET.encode(),
        message.encode(),
        hashlib.sha256
    ).hexdigest()
    return signature

def test_health_check():
    """测试健康检查"""
    print("\n" + "="*50)
    print("测试 1: 健康检查")
    print("="*50)
    
    try:
        response = requests.get(f"{SERVER_URL}/api/health")
        print(f"状态码: {response.status_code}")
        print(f"响应: {json.dumps(response.json(), indent=2, ensure_ascii=False)}")
        
        if response.status_code == 200:
            print("✅ 健康检查通过")
            return True
        else:
            print("❌ 健康检查失败")
            return False
    except Exception as e:
        print(f"❌ 连接失败: {e}")
        return False

def test_request_license():
    """测试申请许可证"""
    print("\n" + "="*50)
    print("测试 2: 申请许可证")
    print("="*50)
    
    machine_code = "TEST-MACHINE-CODE-123456"
    timestamp = int(time.time())
    nonce = "test_nonce_" + str(timestamp)
    signature = generate_signature(machine_code, timestamp, nonce)
    
    data = {
        "machine_code": machine_code,
        "timestamp": timestamp,
        "nonce": nonce,
        "signature": signature,
        "user_info": "test@example.com"
    }
    
    print(f"请求数据: {json.dumps(data, indent=2)}")
    
    try:
        response = requests.post(
            f"{SERVER_URL}/api/license/request",
            json=data,
            headers={"Content-Type": "application/json"}
        )
        
        print(f"\n状态码: {response.status_code}")
        result = response.json()
        print(f"响应: {json.dumps(result, indent=2, ensure_ascii=False)}")
        
        if response.status_code == 200 and result.get('success'):
            print("✅ 许可证申请成功")
            return result.get('license_key')
        else:
            print("❌ 许可证申请失败")
            return None
    except Exception as e:
        print(f"❌ 请求失败: {e}")
        return None

def test_verify_license(machine_code, license_key):
    """测试验证许可证"""
    print("\n" + "="*50)
    print("测试 3: 验证许可证")
    print("="*50)
    
    timestamp = int(time.time())
    nonce = "verify_nonce_" + str(timestamp)
    signature = generate_signature(machine_code, timestamp, nonce)
    
    data = {
        "machine_code": machine_code,
        "license_key": license_key,
        "timestamp": timestamp,
        "nonce": nonce,
        "signature": signature
    }
    
    print(f"请求数据: {json.dumps(data, indent=2)}")
    
    try:
        response = requests.post(
            f"{SERVER_URL}/api/license/verify",
            json=data,
            headers={"Content-Type": "application/json"}
        )
        
        print(f"\n状态码: {response.status_code}")
        result = response.json()
        print(f"响应: {json.dumps(result, indent=2, ensure_ascii=False)}")
        
        if response.status_code == 200 and result.get('valid'):
            print("✅ 许可证验证成功")
            return True
        else:
            print("❌ 许可证验证失败")
            return False
    except Exception as e:
        print(f"❌ 请求失败: {e}")
        return False

def test_license_info(machine_code):
    """测试查询许可证信息"""
    print("\n" + "="*50)
    print("测试 4: 查询许可证信息")
    print("="*50)
    
    timestamp = int(time.time())
    nonce = "info_nonce_" + str(timestamp)
    signature = generate_signature(machine_code, timestamp, nonce)
    
    params = {
        "machine_code": machine_code,
        "timestamp": timestamp,
        "nonce": nonce,
        "signature": signature
    }
    
    try:
        response = requests.get(
            f"{SERVER_URL}/api/license/info",
            params=params
        )
        
        print(f"状态码: {response.status_code}")
        result = response.json()
        print(f"响应: {json.dumps(result, indent=2, ensure_ascii=False)}")
        
        if response.status_code == 200 and result.get('success'):
            print("✅ 查询许可证信息成功")
            return True
        else:
            print("❌ 查询许可证信息失败")
            return False
    except Exception as e:
        print(f"❌ 请求失败: {e}")
        return False

def main():
    """主测试流程"""
    print("\n" + "="*60)
    print("      许可证服务器 API 测试工具")
    print("="*60)
    print(f"服务器地址: {SERVER_URL}")
    print(f"应用密钥: {APP_SECRET}")
    print("="*60)
    
    # 测试 1: 健康检查
    if not test_health_check():
        print("\n❌ 服务器未运行或无法访问！")
        print("请先启动服务器: python secure_license_server.py")
        return
    
    # 测试 2: 申请许可证
    machine_code = "TEST-MACHINE-CODE-123456"
    license_key = test_request_license()
    
    if not license_key:
        print("\n⚠️  跳过后续测试（许可证申请失败）")
        return
    
    # 等待一秒（避免时间戳太接近）
    time.sleep(1)
    
    # 测试 3: 验证许可证
    test_verify_license(machine_code, license_key)
    
    # 等待一秒
    time.sleep(1)
    
    # 测试 4: 查询许可证信息
    test_license_info(machine_code)
    
    # 总结
    print("\n" + "="*60)
    print("测试完成！")
    print("="*60)
    print("\n💡 提示：")
    print("  - 如果测试失败，检查 APP_SECRET 是否与服务器一致")
    print("  - 生产环境请使用 HTTPS")
    print("  - 查看服务器日志了解详细错误信息")
    print("")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n测试被用户中断")
    except Exception as e:
        print(f"\n❌ 测试出错: {e}")
