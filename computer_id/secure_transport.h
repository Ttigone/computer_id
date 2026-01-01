#pragma once

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QMessageAuthenticationCode>
#include <QString>

/// <summary>
/// 安全加密传输模块
/// 实现多层加密防护，防止中间人攻击和重放攻击
/// </summary>
class SecureTransport {
 public:
  SecureTransport();
  ~SecureTransport();

  /// <summary>
  /// 设置应用密钥（客户端和服务端需要保持一致）
  /// 生产环境应使用更复杂的密钥，并且每个应用唯一
  /// </summary>
  static void setAppSecret(const QString& secret);

  /// <summary>
  /// 加密机器码（使用多层加密）
  /// 1. 添加时间戳（防重放攻击）
  /// 2. 添加随机盐值（防彩虹表攻击）
  /// 3. 使用 HMAC-SHA256 签名（防篡改）
  /// 4. Base64 编码（便于传输）
  /// </summary>
  /// <param name="machineCode">原始机器码</param>
  /// <returns>加密后的数据包（JSON格式）</returns>
  static QString encryptMachineCode(const QString& machineCode);

  /// <summary>
  /// 验证并解密机器码（服务端使用）
  /// </summary>
  /// <param name="encryptedData">加密的数据包</param>
  /// <param name="maxAgeSeconds">最大允许时间差（秒），防止重放攻击</param>
  /// <returns>解密后的机器码，失败返回空字符串</returns>
  static QString decryptMachineCode(const QString& encryptedData,
                                    int maxAgeSeconds = 300);

  /// <summary>
  /// 生成请求签名（HMAC-SHA256）
  /// 用于验证请求的完整性和来源
  /// </summary>
  /// <param name="data">要签名的数据</param>
  /// <param name="timestamp">时间戳</param>
  /// <returns>签名字符串</returns>
  static QString generateSignature(const QString& data, qint64 timestamp);

  /// <summary>
  /// 验证请求签名
  /// </summary>
  /// <param name="data">原始数据</param>
  /// <param name="timestamp">时间戳</param>
  /// <param name="signature">签名</param>
  /// <returns>true=签名有效，false=签名无效</returns>
  static bool verifySignature(const QString& data, qint64 timestamp,
                              const QString& signature);

  /// <summary>
  /// 生成随机盐值
  /// </summary>
  static QString generateSalt(int length = 16);

  /// <summary>
  /// AES 加密（对称加密）
  /// </summary>
  static QByteArray aesEncrypt(const QByteArray& data, const QByteArray& key);

  /// <summary>
  /// AES 解密
  /// </summary>
  static QByteArray aesDecrypt(const QByteArray& data, const QByteArray& key);

  /// <summary>
  /// RSA 公钥加密（非对称加密）
  /// 更安全但性能较低，适合加密密钥本身
  /// </summary>
  static QByteArray rsaEncrypt(const QByteArray& data,
                               const QString& publicKeyPem);

  /// <summary>
  /// RSA 私钥解密
  /// </summary>
  static QByteArray rsaDecrypt(const QByteArray& data,
                               const QString& privateKeyPem);

 private:
  static QString s_appSecret;     // 应用密钥
  static QByteArray getAesKey();  // 从应用密钥派生 AES 密钥
};

/// <summary>
/// 安全传输协议包装器
/// 自动处理加密/解密和签名
/// </summary>
class SecurePacket {
 public:
  QString machineCode;  // 机器码
  qint64 timestamp;     // 时间戳
  QString nonce;        // 随机数（防重放）
  QString signature;    // 签名

  /// <summary>
  /// 创建安全数据包
  /// </summary>
  static SecurePacket create(const QString& machineCode);

  /// <summary>
  /// 转换为 JSON 字符串
  /// </summary>
  QString toJson() const;

  /// <summary>
  /// 从 JSON 字符串解析
  /// </summary>
  static SecurePacket fromJson(const QString& jsonStr);

  /// <summary>
  /// 验证数据包的有效性
  /// </summary>
  bool verify(int maxAgeSeconds = 300) const;
};
