#pragma once

#include <functional>
#include <map>
#include <string>

/// <summary>
/// 纯 C++ HTTP/HTTPS 客户端（使用 libcurl）
/// 不依赖 Qt，可在任何 C++ 项目中使用
/// </summary>
class HttpClientCpp {
 public:
  /// <summary>
  /// HTTP 响应结构
  /// </summary>
  struct Response {
    int statusCode;                              // HTTP 状态码
    std::string body;                            // 响应体
    std::map<std::string, std::string> headers;  // 响应头
    std::string error;                           // 错误信息
    bool success;                                // 是否成功
  };

  /// <summary>
  /// 回调函数类型
  /// </summary>
  using ResponseCallback = std::function<void(const Response&)>;

  HttpClientCpp();
  ~HttpClientCpp();

  /// <summary>
  /// 设置超时时间（秒）
  /// </summary>
  void setTimeout(int seconds);

  /// <summary>
  /// 添加自定义请求头
  /// </summary>
  void addHeader(const std::string& key, const std::string& value);

  /// <summary>
  /// 清空所有自定义请求头
  /// </summary>
  void clearHeaders();

  /// <summary>
  /// 启用/禁用 SSL 证书验证（默认启用）
  /// </summary>
  void setVerifySSL(bool verify);

  /// <summary>
  /// GET 请求（同步）
  /// </summary>
  Response get(const std::string& url);

  /// <summary>
  /// POST 请求（同步）
  /// </summary>
  Response post(const std::string& url, const std::string& data,
                const std::string& contentType = "application/json");

  /// <summary>
  /// GET 请求（异步）- 需要自己实现线程管理
  /// </summary>
  void getAsync(const std::string& url, ResponseCallback callback);

  /// <summary>
  /// POST 请求（异步）
  /// </summary>
  void postAsync(const std::string& url, const std::string& data,
                 ResponseCallback callback,
                 const std::string& contentType = "application/json");

 private:
  int m_timeout;
  bool m_verifySSL;
  std::map<std::string, std::string> m_headers;

  // 内部执行请求的方法
  Response performRequest(const std::string& url, const std::string& method,
                          const std::string& data = "");
};

/// <summary>
/// 授权客户端（纯 C++ 实现）
/// 使用 HttpClientCpp + SecureTransportCpp
/// </summary>
class LicenseClientCpp {
 public:
  LicenseClientCpp(const std::string& serverUrl);
  ~LicenseClientCpp();

  /// <summary>
  /// 设置应用密钥
  /// </summary>
  void setAppSecret(const std::string& secret);

  /// <summary>
  /// 请求授权（同步）
  /// </summary>
  struct LicenseResponse {
    bool success;
    std::string licenseKey;
    std::string message;
    std::string expiresAt;
    std::string error;  // 错误信息
  };
  LicenseResponse requestLicense(const std::string& machineCode,
                                 const std::string& userInfo = "");

  /// <summary>
  /// 验证授权（同步）
  /// </summary>
  struct VerifyResponse {
    bool valid;
    std::string message;
    std::string expiresAt;
    std::string error;  // 错误信息
  };
  VerifyResponse verifyLicense(const std::string& machineCode,
                               const std::string& licenseKey);

  /// <summary>
  /// 获取许可证信息（同步）
  /// </summary>
  struct LicenseInfo {
    bool success;
    std::string status;
    std::string userInfo;
    std::string createdAt;
    std::string expiresAt;
    std::string lastVerified;
  };
  LicenseInfo getLicenseInfo(const std::string& machineCode);

 private:
  std::string m_serverUrl;
  HttpClientCpp m_httpClient;
};
