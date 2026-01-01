#include "secure_transport.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageAuthenticationCode>
#include <QRandomGenerator>

QString SecureTransport::s_appSecret = "DEFAULT_APP_SECRET_2026_CHANGE_THIS";

SecureTransport::SecureTransport() {}

SecureTransport::~SecureTransport() {}

void SecureTransport::setAppSecret(const QString& secret) {
  s_appSecret = secret;
}

QString SecureTransport::generateSalt(int length) {
  QString salt;
  const QString chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

  for (int i = 0; i < length; ++i) {
    quint32 random = QRandomGenerator::global()->generate();
    salt.append(chars[random % chars.length()]);
  }

  return salt;
}

QString SecureTransport::generateSignature(const QString& data,
                                           qint64 timestamp) {
  // 组合数据：机器码 + 时间戳 + 应用密钥
  QString message = data + QString::number(timestamp) + s_appSecret;

  // 使用 HMAC-SHA256 生成签名
  QByteArray key = s_appSecret.toUtf8();
  QByteArray messageData = message.toUtf8();

  QByteArray signature = QMessageAuthenticationCode::hash(
      messageData, key, QCryptographicHash::Sha256);

  // 转换为十六进制字符串
  return QString(signature.toHex());
}

bool SecureTransport::verifySignature(const QString& data, qint64 timestamp,
                                      const QString& signature) {
  QString expectedSignature = generateSignature(data, timestamp);
  return signature == expectedSignature;
}

QString SecureTransport::encryptMachineCode(const QString& machineCode) {
  // 1. 生成时间戳（防重放攻击）
  qint64 timestamp = QDateTime::currentSecsSinceEpoch();

  // 2. 生成随机nonce（每次请求唯一）
  QString nonce = generateSalt(16);

  // 3. 组合数据
  QString combinedData =
      machineCode + "|" + QString::number(timestamp) + "|" + nonce;

  // 4. 生成签名（防篡改）
  QString signature = generateSignature(combinedData, timestamp);

  // 5. 构建 JSON 数据包
  QJsonObject json;
  json["machine_code"] = machineCode;
  json["timestamp"] = timestamp;
  json["nonce"] = nonce;
  json["signature"] = signature;

  // 6. 可选：对整个 JSON 进行 Base64 编码（混淆）
  QJsonDocument doc(json);
  QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
  QString base64Data = QString(jsonData.toBase64());

  return base64Data;
}

QString SecureTransport::decryptMachineCode(const QString& encryptedData,
                                            int maxAgeSeconds) {
  try {
    // 1. Base64 解码
    QByteArray jsonData = QByteArray::fromBase64(encryptedData.toUtf8());
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);

    if (!doc.isObject()) {
      return "";
    }

    QJsonObject json = doc.object();

    // 2. 提取字段
    QString machineCode = json["machine_code"].toString();
    qint64 timestamp = json["timestamp"].toVariant().toLongLong();
    QString nonce = json["nonce"].toString();
    QString signature = json["signature"].toString();

    // 3. 验证时间戳（防重放攻击）
    qint64 currentTime = QDateTime::currentSecsSinceEpoch();
    if (currentTime - timestamp > maxAgeSeconds) {
      // 请求过期
      return "";
    }

    // 4. 验证签名（防篡改）
    QString combinedData =
        machineCode + "|" + QString::number(timestamp) + "|" + nonce;
    if (!verifySignature(combinedData, timestamp, signature)) {
      // 签名无效
      return "";
    }

    // 5. 验证通过，返回机器码
    return machineCode;
  } catch (...) {
    return "";
  }
}

QByteArray SecureTransport::getAesKey() {
  // 从应用密钥派生 AES 密钥（使用 SHA256）
  QByteArray key = QCryptographicHash::hash(s_appSecret.toUtf8(),
                                            QCryptographicHash::Sha256);

  // AES-256 需要 32 字节密钥
  return key.left(32);
}

QByteArray SecureTransport::aesEncrypt(const QByteArray& data,
                                       const QByteArray& key) {
  // 注意：这是一个简化的实现
  // 生产环境应使用 OpenSSL 的完整 AES 实现，包括 IV（初始化向量）

  // 使用 OpenSSL EVP 接口进行 AES-256-CBC 加密
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return QByteArray();

  // 初始化向量（IV）
  unsigned char iv[AES_BLOCK_SIZE];
  for (int i = 0; i < AES_BLOCK_SIZE; i++)
    iv[i] = QRandomGenerator::global()->generate() % 256;

  // 初始化加密操作
  if (EVP_EncryptInit_ex(
          ctx, EVP_aes_256_cbc(), nullptr,
          reinterpret_cast<const unsigned char*>(key.constData()), iv) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return QByteArray();
  }

  // 加密数据
  QByteArray encrypted;
  encrypted.resize(data.size() + AES_BLOCK_SIZE);

  int len = 0;
  int ciphertext_len = 0;

  if (EVP_EncryptUpdate(
          ctx, reinterpret_cast<unsigned char*>(encrypted.data()), &len,
          reinterpret_cast<const unsigned char*>(data.constData()),
          data.size()) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return QByteArray();
  }
  ciphertext_len = len;

  if (EVP_EncryptFinal_ex(
          ctx, reinterpret_cast<unsigned char*>(encrypted.data()) + len,
          &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return QByteArray();
  }
  ciphertext_len += len;

  encrypted.resize(ciphertext_len);

  EVP_CIPHER_CTX_free(ctx);

  // 将 IV 附加到密文前面
  QByteArray result(reinterpret_cast<const char*>(iv), AES_BLOCK_SIZE);
  result.append(encrypted);

  return result;
}

QByteArray SecureTransport::aesDecrypt(const QByteArray& data,
                                       const QByteArray& key) {
  if (data.size() < AES_BLOCK_SIZE) return QByteArray();

  // 提取 IV
  unsigned char iv[AES_BLOCK_SIZE];
  memcpy(iv, data.constData(), AES_BLOCK_SIZE);

  // 提取密文
  QByteArray ciphertext = data.mid(AES_BLOCK_SIZE);

  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return QByteArray();

  // 初始化解密操作
  if (EVP_DecryptInit_ex(
          ctx, EVP_aes_256_cbc(), nullptr,
          reinterpret_cast<const unsigned char*>(key.constData()), iv) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return QByteArray();
  }

  // 解密数据
  QByteArray decrypted;
  decrypted.resize(ciphertext.size());

  int len = 0;
  int plaintext_len = 0;

  if (EVP_DecryptUpdate(
          ctx, reinterpret_cast<unsigned char*>(decrypted.data()), &len,
          reinterpret_cast<const unsigned char*>(ciphertext.constData()),
          ciphertext.size()) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return QByteArray();
  }
  plaintext_len = len;

  if (EVP_DecryptFinal_ex(
          ctx, reinterpret_cast<unsigned char*>(decrypted.data()) + len,
          &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return QByteArray();
  }
  plaintext_len += len;

  decrypted.resize(plaintext_len);

  EVP_CIPHER_CTX_free(ctx);

  return decrypted;
}

// ============================================================================
// SecurePacket 实现
// ============================================================================

SecurePacket SecurePacket::create(const QString& machineCode) {
  SecurePacket packet;
  packet.machineCode = machineCode;
  packet.timestamp = QDateTime::currentSecsSinceEpoch();
  packet.nonce = SecureTransport::generateSalt(16);

  // 生成签名
  QString combinedData = machineCode + "|" + QString::number(packet.timestamp) +
                         "|" + packet.nonce;
  packet.signature =
      SecureTransport::generateSignature(combinedData, packet.timestamp);

  return packet;
}

QString SecurePacket::toJson() const {
  QJsonObject json;
  json["machine_code"] = machineCode;
  json["timestamp"] = timestamp;
  json["nonce"] = nonce;
  json["signature"] = signature;

  QJsonDocument doc(json);
  return QString(doc.toJson(QJsonDocument::Compact));
}

SecurePacket SecurePacket::fromJson(const QString& jsonStr) {
  SecurePacket packet;

  QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
  if (doc.isObject()) {
    QJsonObject json = doc.object();
    packet.machineCode = json["machine_code"].toString();
    packet.timestamp = json["timestamp"].toVariant().toLongLong();
    packet.nonce = json["nonce"].toString();
    packet.signature = json["signature"].toString();
  }

  return packet;
}

bool SecurePacket::verify(int maxAgeSeconds) const {
  // 1. 验证时间戳
  qint64 currentTime = QDateTime::currentSecsSinceEpoch();
  if (currentTime - timestamp > maxAgeSeconds) {
    return false;  // 请求过期
  }

  // 2. 验证签名
  QString combinedData =
      machineCode + "|" + QString::number(timestamp) + "|" + nonce;
  return SecureTransport::verifySignature(combinedData, timestamp, signature);
}
