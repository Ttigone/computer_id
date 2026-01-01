# Windows 机器码授权系统

一个完整的基于硬件信息的 Windows 软件授权系统。

## 功能特性

- ✅ 获取唯一的机器标识码（基于 CPU、主板、硬盘序列号）
- ✅ SHA256 加密保证安全性
- ✅ 客户端授权验证
- ✅ 服务端许可证生成
- ✅ 简单易用的命令行接口

## 技术实现

### 硬件信息采集
- **CPU ID**: 通过 WMI 查询 `Win32_Processor.ProcessorId`
- **主板序列号**: 通过 WMI 查询 `Win32_BaseBoard.SerialNumber`
- **硬盘序列号**: 通过 WMI 查询 `Win32_DiskDrive.SerialNumber`

### 机器码生成
组合多个硬件信息后使用 SHA256 哈希算法生成64位十六进制字符串作为唯一标识。

### 授权验证
基于文件的许可证系统，使用双重哈希防止反向推导。

## 使用流程

### 1. 客户端获取机器码

```bash
computer_id.exe --get-code
```

输出示例：
```
======================================
  获取机器码
======================================

硬件信息:
  CPU ID:       BFEBFBFF000906E9
  主板序列号:   /0123456789/
  硬盘序列号:   WD-WCC4E0123456

机器码: a1b2c3d4e5f67890abcdef1234567890abcdef1234567890abcdef1234567890

请将以上机器码发送给服务端进行授权申请。
```

### 2. 服务端生成许可证

客户端提供机器码后，在服务端运行：

```bash
computer_id.exe --generate a1b2c3d4e5f67890abcdef1234567890abcdef1234567890abcdef1234567890
```

这将生成 `license.dat` 文件，将此文件发送给客户端。

### 3. 客户端验证授权

客户端收到 `license.dat` 后，放在程序目录下，运行：

```bash
computer_id.exe --verify
```

输出示例：
```
======================================
  验证授权
======================================

当前机器码: a1b2c3d4e5f67890abcdef1234567890abcdef1234567890abcdef1234567890
正在验证许可证...

[成功] ✓ 授权验证通过！软件可以正常使用。
```

## 集成到你的软件

在你的软件启动代码中添加：

```cpp
#include "win_product.h"

int main()
{
    // 验证授权
    LicenseManager licMgr;
    
    if (!licMgr.VerifyLicense("license.dat"))
    {
        printf("软件未授权！\n");
        printf("您的机器码是: %s\n", licMgr.GetMachineCode().c_str());
        printf("请联系服务端获取授权。\n");
        return 1;
    }
    
    // 授权通过，继续执行软件逻辑
    printf("软件已授权，正常启动...\n");
    
    // 你的软件代码...
    
    return 0;
}
```

## 安全说明

1. **密钥保护**: 修改 `DEFAULT_SECRET_KEY_2026` 为你自己的密钥，并妥善保管
2. **通信加密**: 实际生产环境建议通过 HTTPS 传输机器码和许可证
3. **文件保护**: 可以对 `license.dat` 文件进行额外加密
4. **时间验证**: 可以在许可证中添加有效期验证
5. **在线验证**: 可以扩展为联网验证模式

## 高级功能扩展

### 添加有效期验证

可以修改 `GenerateLicenseFile` 函数，在许可证中包含时间戳：

```cpp
// 许可证格式: 机器码|过期时间戳
std::string licenseData = machineCode + "|" + std::to_string(expiryTimestamp);
std::string licenseContent = Sha256(licenseData + secretKey);
```

### 添加网络验证

```cpp
bool VerifyLicenseOnline(const std::string& machineCode)
{
    // 向服务器发送验证请求
    // 使用 WinHTTP 或 curl 库
    // POST https://yourserver.com/api/verify
    // Body: {"machine_code": "xxx"}
    
    // 解析服务器响应
    // 返回验证结果
}
```

### 添加特征码保护

在许可证中加入软件版本、功能模块等信息，实现更细粒度的授权控制。

## 编译说明

需要链接以下库：
- `wbemuuid.lib` - WMI 支持
- `advapi32.lib` - 加密 API 支持

Visual Studio 项目配置：
- 平台: Windows
- 字符集: 多字节或 Unicode
- C++ 标准: C++11 或更高

## 注意事项

1. 硬件更换后机器码会改变，需要重新授权
2. 虚拟机环境可能无法获取某些硬件信息
3. 需要管理员权限才能访问某些 WMI 信息（一般不需要）
4. 建议采集多个硬件信息以提高稳定性

## 授权流程图

```
客户端                           服务端
   |                               |
   |  1. 获取机器码                |
   |----------------------------->|
   |                              |
   |  2. 发送机器码请求授权        |
   |----------------------------->|
   |                              |
   |                    3. 验证并生成许可证
   |                              |
   |  4. 返回 license.dat         |
   |<-----------------------------|
   |                              |
   |  5. 验证许可证                |
   |  6. 运行软件                  |
```

## 许可证

MIT License - 可自由修改和商用
