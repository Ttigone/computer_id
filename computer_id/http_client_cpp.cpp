#include "http_client_cpp.h"

#include <curl/curl.h>

#include <sstream>
#include <thread>

#include "secure_transport_cpp.h"

// libcurl 回调函数
static size_t WriteCallback(void* contents, size_t size, size_t nmemb,
                            void* userp) {
  ((std::string*)userp)->append((char*)contents, size * nmemb);
  return size * nmemb;
}

static size_t HeaderCallback(char* buffer, size_t size, size_t nitems,
                             void* userdata) {
  size_t numbytes = size * nitems;
  std::string header(buffer, numbytes);

  auto* headers = static_cast<std::map<std::string, std::string>*>(userdata);

  size_t separator = header.find(':');
  if (separator != std::string::npos) {
    std::string key = header.substr(0, separator);
    std::string value = header.substr(separator + 1);

    // 去除空格
    value.erase(0, value.find_first_not_of(" \t\r\n"));
    value.erase(value.find_last_not_of(" \t\r\n") + 1);

    (*headers)[key] = value;
  }

  return numbytes;
}

// ============================================================================
// HttpClientCpp 实现
// ============================================================================

HttpClientCpp::HttpClientCpp() : m_timeout(30), m_verifySSL(true) {
  curl_global_init(CURL_GLOBAL_DEFAULT);
}

HttpClientCpp::~HttpClientCpp() { curl_global_cleanup(); }

void HttpClientCpp::setTimeout(int seconds) { m_timeout = seconds; }

void HttpClientCpp::addHeader(const std::string& key,
                              const std::string& value) {
  m_headers[key] = value;
}

void HttpClientCpp::clearHeaders() { m_headers.clear(); }

void HttpClientCpp::setVerifySSL(bool verify) { m_verifySSL = verify; }

HttpClientCpp::Response HttpClientCpp::get(const std::string& url) {
  return performRequest(url, "GET");
}

HttpClientCpp::Response HttpClientCpp::post(const std::string& url,
                                            const std::string& data,
                                            const std::string& contentType) {
  m_headers["Content-Type"] = contentType;
  return performRequest(url, "POST", data);
}

void HttpClientCpp::getAsync(const std::string& url,
                             ResponseCallback callback) {
  std::thread([this, url, callback]() {
    Response response = get(url);
    if (callback) {
      callback(response);
    }
  }).detach();
}

void HttpClientCpp::postAsync(const std::string& url, const std::string& data,
                              ResponseCallback callback,
                              const std::string& contentType) {
  std::thread([this, url, data, callback, contentType]() {
    Response response = post(url, data, contentType);
    if (callback) {
      callback(response);
    }
  }).detach();
}

HttpClientCpp::Response HttpClientCpp::performRequest(const std::string& url,
                                                      const std::string& method,
                                                      const std::string& data) {
  Response response;
  response.success = false;
  response.statusCode = 0;

  CURL* curl = curl_easy_init();
  if (!curl) {
    response.error = "Failed to initialize CURL";
    return response;
  }

  // 设置 URL
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

  // 设置超时
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, m_timeout);

  // 设置 SSL 验证
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, m_verifySSL ? 1L : 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, m_verifySSL ? 2L : 0L);

  // 设置响应回调
  std::string responseBody;
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

  // 设置响应头回调
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers);

  // 设置请求头
  struct curl_slist* headers = nullptr;
  for (const auto& header : m_headers) {
    std::string headerStr = header.first + ": " + header.second;
    headers = curl_slist_append(headers, headerStr.c_str());
  }

  if (headers) {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  }

  // 设置请求方法
  if (method == "POST") {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.length());
  } else if (method == "GET") {
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
  }

  // 执行请求
  CURLcode res = curl_easy_perform(curl);

  if (res != CURLE_OK) {
    response.error = curl_easy_strerror(res);
    response.success = false;
  } else {
    // 获取状态码
    long statusCode;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &statusCode);
    response.statusCode = static_cast<int>(statusCode);
    response.body = responseBody;
    response.success = (statusCode >= 200 && statusCode < 300);
  }

  // 清理
  if (headers) {
    curl_slist_free_all(headers);
  }
  curl_easy_cleanup(curl);

  return response;
}

// ============================================================================
// LicenseClientCpp 实现
// ============================================================================

LicenseClientCpp::LicenseClientCpp(const std::string& serverUrl)
    : m_serverUrl(serverUrl) {
  // 添加默认请求头
  m_httpClient.addHeader("Content-Type", "application/json");
  m_httpClient.addHeader("User-Agent", "LicenseClientCpp/1.0");
}

LicenseClientCpp::~LicenseClientCpp() {}

void LicenseClientCpp::setAppSecret(const std::string& secret) {
  SecureTransportCpp::setAppSecret(secret);
}

// 简单的 JSON 解析辅助函数
static std::string extractJsonValue(const std::string& json,
                                    const std::string& key) {
  std::string searchKey = "\"" + key + "\":";
  size_t pos = json.find(searchKey);
  if (pos == std::string::npos) return "";

  size_t valueStart = json.find('"', pos + searchKey.length());
  if (valueStart == std::string::npos) return "";

  size_t valueEnd = json.find('"', valueStart + 1);
  if (valueEnd == std::string::npos) return "";

  return json.substr(valueStart + 1, valueEnd - valueStart - 1);
}

static bool extractJsonBool(const std::string& json, const std::string& key) {
  std::string searchKey = "\"" + key + "\":";
  size_t pos = json.find(searchKey);
  if (pos == std::string::npos) return false;

  size_t truePos = json.find("true", pos);
  size_t falsePos = json.find("false", pos);

  if (truePos != std::string::npos && truePos < pos + 20) return true;

  return false;
}

LicenseClientCpp::LicenseResponse LicenseClientCpp::requestLicense(
    const std::string& machineCode, const std::string& userInfo) {
  LicenseResponse result;
  result.success = false;

  // 创建安全数据包
  SecurePacketCpp packet = SecurePacketCpp::create(machineCode);
  std::string securePacket = packet.toJson();

  // Base64 编码
  std::string base64Packet = SecureTransportCpp::base64Encode(securePacket);

  // 构建请求 JSON
  std::ostringstream oss;
  oss << "{"
      << "\"secure_packet\":\"" << base64Packet << "\","
      << "\"user_info\":\"" << userInfo << "\","
      << "\"action\":\"request\""
      << "}";

  std::string requestBody = oss.str();

  // 发送请求
  HttpClientCpp::Response response =
      m_httpClient.post(m_serverUrl + "/license/request", requestBody);

  if (response.success) {
    result.success = extractJsonBool(response.body, "success");
    result.licenseKey = extractJsonValue(response.body, "license_key");
    result.message = extractJsonValue(response.body, "message");
    result.expiresAt = extractJsonValue(response.body, "expires_at");
  } else {
    result.message = response.error;
  }

  return result;
}

LicenseClientCpp::VerifyResponse LicenseClientCpp::verifyLicense(
    const std::string& machineCode, const std::string& licenseKey) {
  VerifyResponse result;
  result.valid = false;

  // 创建安全数据包
  SecurePacketCpp packet = SecurePacketCpp::create(machineCode);
  std::string securePacket = packet.toJson();
  std::string base64Packet = SecureTransportCpp::base64Encode(securePacket);

  // 构建请求 JSON
  std::ostringstream oss;
  oss << "{"
      << "\"secure_packet\":\"" << base64Packet << "\","
      << "\"license_key\":\"" << licenseKey << "\","
      << "\"action\":\"verify\""
      << "}";

  std::string requestBody = oss.str();

  // 发送请求
  HttpClientCpp::Response response =
      m_httpClient.post(m_serverUrl + "/license/verify", requestBody);

  if (response.success) {
    result.valid = extractJsonBool(response.body, "valid");
    result.message = extractJsonValue(response.body, "message");
    result.expiresAt = extractJsonValue(response.body, "expires_at");
  } else {
    result.message = response.error;
  }

  return result;
}

LicenseClientCpp::LicenseInfo LicenseClientCpp::getLicenseInfo(
    const std::string& machineCode) {
  LicenseInfo result;
  result.success = false;

  // 创建安全数据包
  SecurePacketCpp packet = SecurePacketCpp::create(machineCode);
  std::string securePacket = packet.toJson();
  std::string base64Packet = SecureTransportCpp::base64Encode(securePacket);

  // 构建请求 JSON
  std::ostringstream oss;
  oss << "{"
      << "\"secure_packet\":\"" << base64Packet << "\","
      << "\"action\":\"info\""
      << "}";

  std::string requestBody = oss.str();

  // 发送请求
  HttpClientCpp::Response response =
      m_httpClient.post(m_serverUrl + "/license/info", requestBody);

  if (response.success) {
    result.success = extractJsonBool(response.body, "success");

    // 提取 license_info 对象中的字段
    size_t infoStart = response.body.find("\"license_info\":");
    if (infoStart != std::string::npos) {
      std::string infoJson = response.body.substr(infoStart);

      result.status = extractJsonValue(infoJson, "status");
      result.userInfo = extractJsonValue(infoJson, "user_info");
      result.createdAt = extractJsonValue(infoJson, "created_at");
      result.expiresAt = extractJsonValue(infoJson, "expires_at");
      result.lastVerified = extractJsonValue(infoJson, "last_verified");
    }
  }

  return result;
}
