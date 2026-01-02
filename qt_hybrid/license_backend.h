#pragma once

#include <string>

#include "../computer_id/http_client_cpp.h"

/// <summary>
/// 授权管理后端（纯 C++ 实现，不依赖 Qt）
/// 封装所有业务逻辑，供 Qt UI 调用
/// </summary>
class LicenseBackend {
 public:
  LicenseBackend();
  ~LicenseBackend();

  /// <summary>
  /// 设置服务器地址
  /// </summary>
  void setServerUrl(const std::string& url);

  /// <summary>
  /// 设置应用密钥
  /// </summary>
  void setAppSecret(const std::string& secret);

  /// <summary>
  /// 获取机器码（使用纯 C++ 实现）
  /// </summary>
  std::string getMachineCode();

  /// <summary>
  /// 请求授权
  /// </summary>
  LicenseClientCpp::LicenseResponse requestLicense(
      const std::string& machineCode, const std::string& userInfo = "");

  /// <summary>
  /// 验证授权
  /// </summary>
  LicenseClientCpp::VerifyResponse verifyLicense(const std::string& machineCode,
                                                 const std::string& licenseKey);

  /// <summary>
  /// 获取许可证信息
  /// </summary>
  LicenseClientCpp::LicenseInfo getLicenseInfo(const std::string& machineCode);

  /// <summary>
  /// 保存许可证到本地文件
  /// </summary>
  static bool saveLicenseToFile(const std::string& licenseKey,
                                const std::string& filePath = "license.dat");

  /// <summary>
  /// 从本地文件加载许可证
  /// </summary>
  static std::string loadLicenseFromFile(
      const std::string& filePath = "license.dat");

  /// <summary>
  /// 删除本地许可证文件
  /// </summary>
  static bool deleteLicenseFile(const std::string& filePath = "license.dat");

 private:
  LicenseClientCpp* m_client;
  std::string m_serverUrl;
};
