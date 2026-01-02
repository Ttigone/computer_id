#include "secure_transport_cpp.h"

#include <openssl/aes.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

// 使用 nlohmann/json 库解析 JSON（需要单独安装）
// 或者使用 rapidjson
// 这里提供一个简化的 JSON 处理实现
#include <map>

// 静态成员初始化
std::string SecureTransportCpp::s_appSecret =
    "DEFAULT_APP_SECRET_2026_CHANGE_THIS";

SecureTransportCpp::SecureTransportCpp() {}

SecureTransportCpp::~SecureTransportCpp() {}

void SecureTransportCpp::setAppSecret(const std::string& secret) {
  s_appSecret = secret;
}

// ============================================================================
// Base64 编码/解码（使用 OpenSSL）
// ============================================================================

std::string SecureTransportCpp::base64Encode(
    const std::vector<unsigned char>& data) {
  BIO* bio = BIO_new(BIO_s_mem());
  BIO* b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);  // 不换行
  bio = BIO_push(b64, bio);

  BIO_write(bio, data.data(), static_cast<int>(data.size()));
  BIO_flush(bio);

  // OpenSSL 3.x 兼容性：使用 BIO_get_mem_data 替代直接访问 BUF_MEM
  char* bufferPtr = nullptr;
  long length = BIO_get_mem_data(bio, &bufferPtr);

  std::string result(bufferPtr, length);

  BIO_free_all(bio);

  return result;
}

std::string SecureTransportCpp::base64Encode(const std::string& data) {
  std::vector<unsigned char> vec(data.begin(), data.end());
  return base64Encode(vec);
}

std::vector<unsigned char> SecureTransportCpp::base64Decode(
    const std::string& encoded) {
  BIO* bio =
      BIO_new_mem_buf(encoded.data(), static_cast<int>(encoded.length()));
  BIO* b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  bio = BIO_push(b64, bio);

  std::vector<unsigned char> result(encoded.length());
  int decodedLength =
      BIO_read(bio, result.data(), static_cast<int>(encoded.length()));

  BIO_free_all(bio);

  if (decodedLength > 0) {
    result.resize(decodedLength);
  } else {
    result.clear();
  }

  return result;
}

// ============================================================================
// SHA256 哈希
// ============================================================================

std::string SecureTransportCpp::sha256(const std::string& input) {
  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256_CTX sha256;
  SHA256_Init(&sha256);
  SHA256_Update(&sha256, input.c_str(), input.length());
  SHA256_Final(hash, &sha256);

  std::ostringstream oss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(hash[i]);
  }

  return oss.str();
}

// ============================================================================
// HMAC-SHA256 签名
// ============================================================================

std::string SecureTransportCpp::generateSignature(const std::string& data,
                                                  int64_t timestamp) {
  // 组合数据
  std::string message = data + std::to_string(timestamp) + s_appSecret;

  // HMAC-SHA256
  unsigned char* digest = HMAC(
      EVP_sha256(), s_appSecret.c_str(), static_cast<int>(s_appSecret.length()),
      reinterpret_cast<const unsigned char*>(message.c_str()), message.length(),
      nullptr, nullptr);

  // 转换为十六进制
  std::ostringstream oss;
  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    oss << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(digest[i]);
  }

  return oss.str();
}

bool SecureTransportCpp::verifySignature(const std::string& data,
                                         int64_t timestamp,
                                         const std::string& signature) {
  std::string expectedSignature = generateSignature(data, timestamp);
  return signature == expectedSignature;
}

// ============================================================================
// 随机数生成
// ============================================================================

std::string SecureTransportCpp::generateSalt(int length) {
  const char charset[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  const size_t max_index = sizeof(charset) - 1;

  std::string salt;
  salt.reserve(length);

  unsigned char randomBytes[256];
  RAND_bytes(randomBytes, length);

  for (int i = 0; i < length; i++) {
    salt += charset[randomBytes[i] % max_index];
  }

  return salt;
}

// ============================================================================
// AES 加密/解密
// ============================================================================

std::vector<unsigned char> SecureTransportCpp::aesEncrypt(
    const std::vector<unsigned char>& data, const std::string& key) {
  // 从密钥派生 32 字节的 AES 密钥
  std::string keyHash = sha256(key);
  std::vector<unsigned char> aesKey(keyHash.begin(), keyHash.begin() + 32);

  // 生成随机 IV
  unsigned char iv[AES_BLOCK_SIZE];
  RAND_bytes(iv, AES_BLOCK_SIZE);

  // 创建加密上下文
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return std::vector<unsigned char>();

  // 初始化加密
  if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, aesKey.data(), iv) !=
      1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::vector<unsigned char>();
  }

  // 加密数据
  std::vector<unsigned char> encrypted(data.size() + AES_BLOCK_SIZE);
  int len = 0;
  int ciphertext_len = 0;

  if (EVP_EncryptUpdate(ctx, encrypted.data(), &len, data.data(),
                        static_cast<int>(data.size())) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::vector<unsigned char>();
  }
  ciphertext_len = len;

  if (EVP_EncryptFinal_ex(ctx, encrypted.data() + len, &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::vector<unsigned char>();
  }
  ciphertext_len += len;

  encrypted.resize(ciphertext_len);

  EVP_CIPHER_CTX_free(ctx);

  // 将 IV 附加到密文前面
  std::vector<unsigned char> result(iv, iv + AES_BLOCK_SIZE);
  result.insert(result.end(), encrypted.begin(), encrypted.end());

  return result;
}

std::vector<unsigned char> SecureTransportCpp::aesDecrypt(
    const std::vector<unsigned char>& data, const std::string& key) {
  if (data.size() < AES_BLOCK_SIZE) return std::vector<unsigned char>();

  // 从密钥派生 AES 密钥
  std::string keyHash = sha256(key);
  std::vector<unsigned char> aesKey(keyHash.begin(), keyHash.begin() + 32);

  // 提取 IV
  unsigned char iv[AES_BLOCK_SIZE];
  std::memcpy(iv, data.data(), AES_BLOCK_SIZE);

  // 提取密文
  std::vector<unsigned char> ciphertext(data.begin() + AES_BLOCK_SIZE,
                                        data.end());

  // 创建解密上下文
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  if (!ctx) return std::vector<unsigned char>();

  // 初始化解密
  if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, aesKey.data(), iv) !=
      1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::vector<unsigned char>();
  }

  // 解密数据
  std::vector<unsigned char> decrypted(ciphertext.size());
  int len = 0;
  int plaintext_len = 0;

  if (EVP_DecryptUpdate(ctx, decrypted.data(), &len, ciphertext.data(),
                        static_cast<int>(ciphertext.size())) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::vector<unsigned char>();
  }
  plaintext_len = len;

  if (EVP_DecryptFinal_ex(ctx, decrypted.data() + len, &len) != 1) {
    EVP_CIPHER_CTX_free(ctx);
    return std::vector<unsigned char>();
  }
  plaintext_len += len;

  decrypted.resize(plaintext_len);

  EVP_CIPHER_CTX_free(ctx);

  return decrypted;
}

// ============================================================================
// 简单的 JSON 构建和解析（生产环境建议使用 nlohmann/json）
// ============================================================================

static std::string buildJson(const std::map<std::string, std::string>& data) {
  std::ostringstream oss;
  oss << "{";
  bool first = true;
  for (const auto& pair : data) {
    if (!first) oss << ",";
    oss << "\"" << pair.first << "\":\"" << pair.second << "\"";
    first = false;
  }
  oss << "}";
  return oss.str();
}

static std::map<std::string, std::string> parseJson(const std::string& json) {
  std::map<std::string, std::string> result;

  // 简化的 JSON 解析（仅处理简单的键值对）
  size_t pos = 0;
  while (pos < json.length()) {
    size_t keyStart = json.find('"', pos);
    if (keyStart == std::string::npos) break;

    size_t keyEnd = json.find('"', keyStart + 1);
    if (keyEnd == std::string::npos) break;

    std::string key = json.substr(keyStart + 1, keyEnd - keyStart - 1);

    size_t valueStart = json.find('"', keyEnd + 1);
    if (valueStart == std::string::npos) break;

    size_t valueEnd = json.find('"', valueStart + 1);
    if (valueEnd == std::string::npos) break;

    std::string value = json.substr(valueStart + 1, valueEnd - valueStart - 1);

    result[key] = value;
    pos = valueEnd + 1;
  }

  return result;
}

// ============================================================================
// 加密/解密机器码
// ============================================================================

std::string SecureTransportCpp::encryptMachineCode(
    const std::string& machineCode) {
  // 1. 生成时间戳
  int64_t timestamp = static_cast<int64_t>(std::time(nullptr));

  // 2. 生成随机 nonce
  std::string nonce = generateSalt(16);

  // 3. 组合数据
  std::string combinedData =
      machineCode + "|" + std::to_string(timestamp) + "|" + nonce;

  // 4. 生成签名
  std::string signature = generateSignature(combinedData, timestamp);

  // 5. 构建 JSON
  std::map<std::string, std::string> jsonData;
  jsonData["machine_code"] = machineCode;
  jsonData["timestamp"] = std::to_string(timestamp);
  jsonData["nonce"] = nonce;
  jsonData["signature"] = signature;

  std::string json = buildJson(jsonData);

  // 6. Base64 编码
  return base64Encode(json);
}

std::string SecureTransportCpp::decryptMachineCode(
    const std::string& encryptedData, int maxAgeSeconds) {
  try {
    // 1. Base64 解码
    std::vector<unsigned char> decoded = base64Decode(encryptedData);
    std::string json(decoded.begin(), decoded.end());

    // 2. 解析 JSON
    std::map<std::string, std::string> data = parseJson(json);

    std::string machineCode = data["machine_code"];
    int64_t timestamp = std::stoll(data["timestamp"]);
    std::string nonce = data["nonce"];
    std::string signature = data["signature"];

    // 3. 验证时间戳
    int64_t currentTime = static_cast<int64_t>(std::time(nullptr));
    if (currentTime - timestamp > maxAgeSeconds) {
      return "";  // 过期
    }

    // 4. 验证签名
    std::string combinedData =
        machineCode + "|" + std::to_string(timestamp) + "|" + nonce;
    if (!verifySignature(combinedData, timestamp, signature)) {
      return "";  // 签名无效
    }

    // 5. 返回机器码
    return machineCode;
  } catch (...) {
    return "";
  }
}

// ============================================================================
// SecurePacketCpp 实现
// ============================================================================

SecurePacketCpp SecurePacketCpp::create(const std::string& machineCode) {
  SecurePacketCpp packet;
  packet.machineCode = machineCode;
  packet.timestamp = static_cast<int64_t>(std::time(nullptr));
  packet.nonce = SecureTransportCpp::generateSalt(16);

  // 生成签名
  std::string combinedData =
      machineCode + "|" + std::to_string(packet.timestamp) + "|" + packet.nonce;
  packet.signature =
      SecureTransportCpp::generateSignature(combinedData, packet.timestamp);

  return packet;
}

std::string SecurePacketCpp::toJson() const {
  std::map<std::string, std::string> data;
  data["machine_code"] = machineCode;
  data["timestamp"] = std::to_string(timestamp);
  data["nonce"] = nonce;
  data["signature"] = signature;

  return buildJson(data);
}

SecurePacketCpp SecurePacketCpp::fromJson(const std::string& jsonStr) {
  SecurePacketCpp packet;

  std::map<std::string, std::string> data = parseJson(jsonStr);

  packet.machineCode = data["machine_code"];
  packet.timestamp = std::stoll(data["timestamp"]);
  packet.nonce = data["nonce"];
  packet.signature = data["signature"];

  return packet;
}

bool SecurePacketCpp::verify(int maxAgeSeconds) const {
  // 1. 验证时间戳
  int64_t currentTime = static_cast<int64_t>(std::time(nullptr));
  if (currentTime - timestamp > maxAgeSeconds) {
    return false;
  }

  // 2. 验证签名
  std::string combinedData =
      machineCode + "|" + std::to_string(timestamp) + "|" + nonce;
  return SecureTransportCpp::verifySignature(combinedData, timestamp,
                                             signature);
}
