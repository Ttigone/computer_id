#pragma once

#include <windows.h>

#include <string>

// Windows 机器码获取与授权验证系统

/// <summary>
/// 从 WMI 获取指定类的属性值
/// </summary>
/// <param name="className">WMI 类名，如 Win32_Processor</param>
/// <param name="propertyName">属性名，如 ProcessorId</param>
/// <returns>属性值字符串，失败返回空字符串</returns>
std::string GetWmiProperty(const std::wstring& className,
                           const std::wstring& propertyName);

/// <summary>
/// 获取 CPU 序列号
/// </summary>
/// <returns>CPU ID 字符串</returns>
std::string GetCpuId();

/// <summary>
/// 获取主板序列号
/// </summary>
/// <returns>主板序列号字符串</returns>
std::string GetMotherboardSerial();

/// <summary>
/// 获取硬盘序列号（第一块物理硬盘）
/// </summary>
/// <returns>硬盘序列号字符串</returns>
std::string GetDiskSerial();

/// <summary>
/// 生成机器唯一标识码（基于多个硬件信息的 SHA256 哈希）
/// </summary>
/// <returns>机器码（64位十六进制字符串）</returns>
std::string GenerateMachineCode();

/// <summary>
/// 计算字符串的 SHA256 哈希值
/// </summary>
/// <param name="input">输入字符串</param>
/// <returns>SHA256 哈希值（十六进制字符串）</returns>
std::string Sha256(const std::string& input);

// 授权验证相关

/// <summary>
/// 授权验证类
/// </summary>
class LicenseManager {
 public:
  LicenseManager();
  ~LicenseManager();

  /// <summary>
  /// 获取当前机器码
  /// </summary>
  /// <returns>机器码字符串</returns>
  std::string GetMachineCode() const;

  /// <summary>
  /// 验证授权（从许可证文件读取）
  /// </summary>
  /// <param name="licenseFilePath">许可证文件路径，默认为当前目录的
  /// license.dat</param>
  /// <returns>true=授权有效，false=授权无效或不存在</returns>
  bool VerifyLicense(const std::string& licenseFilePath = "license.dat");

  /// <summary>
  /// 生成许可证文件（仅供服务端使用）
  /// 服务端接收到机器码后，使用此函数生成许可证文件发给客户端
  /// </summary>
  /// <param name="machineCode">客户端提供的机器码</param>
  /// <param name="licenseFilePath">要生成的许可证文件路径</param>
  /// <param name="secretKey">服务端密钥（用于签名）</param>
  /// <returns>true=生成成功，false=失败</returns>
  static bool GenerateLicenseFile(
      const std::string& machineCode, const std::string& licenseFilePath,
      const std::string& secretKey = "DEFAULT_SECRET_KEY_2026");

 private:
  std::string m_machineCode;
};
