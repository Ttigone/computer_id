#include "qt_secure_license_client.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QSslConfiguration>

QtSecureLicenseClient::QtSecureLicenseClient(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_serverUrl("https://localhost:5000/api") {
  connect(m_networkManager, &QNetworkAccessManager::finished, this,
          &QtSecureLicenseClient::onRequestFinished);

  connect(m_networkManager, &QNetworkAccessManager::sslErrors, this,
          &QtSecureLicenseClient::onSslErrors);
}

QtSecureLicenseClient::~QtSecureLicenseClient() {}

void QtSecureLicenseClient::setServerUrl(const QString& baseUrl) {
  m_serverUrl = baseUrl;
}

void QtSecureLicenseClient::setAppSecret(const QString& secret) {
  SecureTransport::setAppSecret(secret);
}

void QtSecureLicenseClient::requestLicense(const QString& machineCode,
                                           const QString& userInfo) {
  QJsonObject extraData;
  extraData["user_info"] = userInfo;
  extraData["action"] = "request";

  sendSecurePostRequest("/license/request", machineCode, extraData,
                        "REQUEST_LICENSE");
}

void QtSecureLicenseClient::verifyLicenseOnline(const QString& machineCode,
                                                const QString& licenseKey) {
  QJsonObject extraData;
  extraData["license_key"] = licenseKey;
  extraData["action"] = "verify";

  sendSecurePostRequest("/license/verify", machineCode, extraData,
                        "VERIFY_LICENSE");
}

void QtSecureLicenseClient::getLicenseInfo(const QString& machineCode) {
  QJsonObject extraData;
  extraData["action"] = "info";

  sendSecurePostRequest("/license/info", machineCode, extraData, "GET_INFO");
}

void QtSecureLicenseClient::sendSecurePostRequest(const QString& endpoint,
                                                  const QString& machineCode,
                                                  const QJsonObject& extraData,
                                                  const QString& requestType) {
  // 1. 创建安全数据包（包含签名和时间戳）
  SecurePacket packet = SecurePacket::create(machineCode);

  // 2. 构建完整的请求数据
  QJsonObject json;

  // 使用加密的数据包
  json["secure_packet"] = packet.toJson();

  // 添加额外数据（非敏感信息）
  for (auto it = extraData.begin(); it != extraData.end(); ++it) {
    json[it.key()] = it.value();
  }

  // 3. 构建请求
  QString url = m_serverUrl + endpoint;
  QNetworkRequest request(url);

  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setHeader(QNetworkRequest::UserAgentHeader,
                    "QtSecureLicenseClient/2.0");

  // 添加自定义请求头（用于额外验证）
  qint64 timestamp = QDateTime::currentSecsSinceEpoch();
  request.setRawHeader("X-Request-Time", QString::number(timestamp).toUtf8());
  request.setRawHeader("X-Client-Version", "2.0");

  // SSL 配置
  QSslConfiguration sslConfig = request.sslConfiguration();
  sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
  sslConfig.setProtocol(QSsl::TlsV1_2OrLater);  // 强制使用 TLS 1.2+
  request.setSslConfiguration(sslConfig);

  // 4. 发送请求
  QJsonDocument doc(json);
  QByteArray jsonData = doc.toJson();

  QNetworkReply* reply = m_networkManager->post(request, jsonData);
  reply->setProperty("requestType", requestType);

  qDebug() << "[SECURE] Sending encrypted request to:" << url;
}

void QtSecureLicenseClient::onRequestFinished(QNetworkReply* reply) {
  reply->deleteLater();

  if (reply->error() != QNetworkReply::NoError) {
    emit networkError(QString("Network error: %1").arg(reply->errorString()));
    return;
  }

  QByteArray responseData = reply->readAll();
  QJsonDocument doc = QJsonDocument::fromJson(responseData);

  if (doc.isNull() || !doc.isObject()) {
    emit networkError("Invalid JSON response");
    return;
  }

  QJsonObject response = doc.object();
  QString requestType = reply->property("requestType").toString();

  // 处理不同类型的响应
  if (requestType == "REQUEST_LICENSE") {
    bool success = response["success"].toBool();
    QString licenseKey = response["license_key"].toString();
    QString message = response["message"].toString();

    emit licenseRequestFinished(success, licenseKey, message);
  } else if (requestType == "VERIFY_LICENSE") {
    bool valid = response["valid"].toBool();
    QString message = response["message"].toString();

    emit licenseVerifyFinished(valid, message);
  } else if (requestType == "GET_INFO") {
    bool success = response["success"].toBool();
    QJsonObject info = response["license_info"].toObject();

    emit licenseInfoReceived(success, info);
  }
}

void QtSecureLicenseClient::onSslErrors(QNetworkReply* reply,
                                        const QList<QSslError>& errors) {
  qWarning() << "[SSL ERROR] SSL Errors occurred:";
  for (const QSslError& error : errors) {
    qWarning() << "  -" << error.errorString();
  }

  // 生产环境不应该忽略 SSL 错误！
  // reply->ignoreSslErrors();  // 仅开发环境

  emit networkError("SSL certificate verification failed");
}
