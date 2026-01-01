#pragma once

#include <ctime>
#include <string>
#include <vector>

/// <summary>
/// 纯 C++ 安全传输模块（不依赖 Qt）
/// 使用标准 C++ + OpenSSL 实现
/// 可以在任何 C++ 项目中使用
/// </summary>
class SecureTransportCpp {
 public:
  SecureTransportCpp();
  ~SecureTransportCpp();

  /// <summary>
  /// 设置应用密钥
  /// </summary>
  static void setAppSecret(const std::string& secret);

  /// <summary>
  /// 生成随机盐值
  /// </summary>
  static std::string generateSalt(int length = 16);

  /// <summary>
  /// 生成 HMAC-SHA256 签名
  /// </summary>
  static std::string generateSignature(const std::string& data,
                                       int64_t timestamp);

  /// <summary>
  /// 验证签名
  /// </summary>
  static bool verifySignature(const std::string& data, int64_t timestamp,
                              const std::string& signature);

  /// <summary>
  /// 加密机器码（返回 Base64 编码的 JSON）
  /// </summary>
  static std::string encryptMachineCode(const std::string& machineCode);

  /// <summary>
  /// 解密机器码
  /// </summary>
  static std::string decryptMachineCode(const std::string& encryptedData,
                                        int maxAgeSeconds = 300);

  /// <summary>
  /// Base64 编码
  /// </summary>
  static std::string base64Encode(const std::vector<unsigned char>& data);
  static std::string base64Encode(const std::string& data);

  /// <summary>
  /// Base64 解码
  /// </summary>
  static std::vector<unsigned char> base64Decode(const std::string& encoded);

  /// <summary>
  /// SHA256 哈希
  /// </summary>
  static std::string sha256(const std::string& input);

  /// <summary>
  /// AES-256-CBC 加密
  /// </summary>
  static std::vector<unsigned char> aesEncrypt(
      const std::vector<unsigned char>& data, const std::string& key);

  /// <summary>
  /// AES-256-CBC 解密
  /// </summary>
  static std::vector<unsigned char> aesDecrypt(
      const std::vector<unsigned char>& data, const std::string& key);

 private:
  static std::string s_appSecret;
};

/// <summary>
/// 安全数据包（纯 C++ 实现）
/// </summary>
struct SecurePacketCpp {
  std::string machineCode;
  int64_t timestamp;
  std::string nonce;
  std::string signature;

  /// <summary>
  /// 创建安全数据包
  /// </summary>
  static SecurePacketCpp create(const std::string& machineCode);

  /// <summary>
  /// 转换为 JSON 字符串
  /// </summary>
  std::string toJson() const;

  /// <summary>
  /// 从 JSON 解析
  /// </summary>
  static SecurePacketCpp fromJson(const std::string& jsonStr);

  /// <summary>
  /// 验证数据包
  /// </summary>
  bool verify(int maxAgeSeconds = 300) const;
};
