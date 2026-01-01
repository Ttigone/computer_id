"""
客户端测试脚本 - 模拟 Qt 客户端请求

用于测试服务端 API 是否正常工作
运行前请确保 license_server.py 已启动
"""

import requests
import json

# 服务器地址
SERVER_URL = "http://localhost:5000/api"

def test_health():
    """测试健康检查"""
    print("\n" + "="*50)
    print("测试: 健康检查")
    print("="*50)
    
    response = requests.get(f"{SERVER_URL}/health")
    print(f"状态码: {response.status_code}")
    print(f"响应: {response.json()}")

def test_request_license():
    """测试申请许可证"""
    print("\n" + "="*50)
    print("测试: 申请许可证")
    print("="*50)
    
    data = {
        "machine_code": "test_machine_12345678abcdefgh",
        "user_info": "test_user@example.com"
    }
    
    print(f"请求数据: {json.dumps(data, indent=2)}")
    
    response = requests.post(
        f"{SERVER_URL}/license/request",
        json=data,
        headers={"Content-Type": "application/json"}
    )
    
    print(f"状态码: {response.status_code}")
    result = response.json()
    print(f"响应: {json.dumps(result, indent=2)}")
    
    return result.get('license_key', '')

def test_verify_license(machine_code, license_key):
    """测试验证许可证"""
    print("\n" + "="*50)
    print("测试: 验证许可证")
    print("="*50)
    
    data = {
        "machine_code": machine_code,
        "license_key": license_key
    }
    
    print(f"请求数据: {json.dumps(data, indent=2)}")
    
    response = requests.post(
        f"{SERVER_URL}/license/verify",
        json=data,
        headers={"Content-Type": "application/json"}
    )
    
    print(f"状态码: {response.status_code}")
    result = response.json()
    print(f"响应: {json.dumps(result, indent=2)}")

def test_get_license_info(machine_code):
    """测试查询许可证信息"""
    print("\n" + "="*50)
    print("测试: 查询许可证信息")
    print("="*50)
    
    data = {
        "machine_code": machine_code
    }
    
    print(f"请求数据: {json.dumps(data, indent=2)}")
    
    response = requests.post(
        f"{SERVER_URL}/license/info",
        json=data,
        headers={"Content-Type": "application/json"}
    )
    
    print(f"状态码: {response.status_code}")
    result = response.json()
    print(f"响应: {json.dumps(result, indent=2)}")

def test_verify_invalid_license():
    """测试验证无效许可证"""
    print("\n" + "="*50)
    print("测试: 验证无效许可证 (应该失败)")
    print("="*50)
    
    data = {
        "machine_code": "test_machine_12345678abcdefgh",
        "license_key": "invalid_license_key_123456"
    }
    
    print(f"请求数据: {json.dumps(data, indent=2)}")
    
    response = requests.post(
        f"{SERVER_URL}/license/verify",
        json=data,
        headers={"Content-Type": "application/json"}
    )
    
    print(f"状态码: {response.status_code}")
    result = response.json()
    print(f"响应: {json.dumps(result, indent=2)}")

if __name__ == "__main__":
    print("="*50)
    print("  License Server API 测试")
    print("="*50)
    print(f"服务器地址: {SERVER_URL}")
    print("请确保 license_server.py 已经运行")
    print("="*50)
    
    try:
        # 1. 健康检查
        test_health()
        
        # 2. 申请许可证
        machine_code = "test_machine_12345678abcdefgh"
        license_key = test_request_license()
        
        # 3. 验证有效许可证
        if license_key:
            test_verify_license(machine_code, license_key)
        
        # 4. 查询许可证信息
        test_get_license_info(machine_code)
        
        # 5. 验证无效许可证
        test_verify_invalid_license()
        
        print("\n" + "="*50)
        print("  所有测试完成!")
        print("="*50)
        
    except requests.exceptions.ConnectionError:
        print("\n[错误] 无法连接到服务器!")
        print("请确保 license_server.py 正在运行")
        print("运行命令: python license_server.py")
    except Exception as e:
        print(f"\n[错误] 测试失败: {str(e)}")
