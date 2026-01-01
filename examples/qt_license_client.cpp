#include "qt_license_client.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QSslConfiguration>

QtLicenseClient::QtLicenseClient(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_serverUrl("https://localhost:5000/api")  // 默认地址
{
  // 连接网络请求完成信号
  connect(m_networkManager, &QNetworkAccessManager::finished, this,
          &QtLicenseClient::onRequestFinished);

  // 处理 SSL 错误（开发环境可能使用自签名证书）
  connect(m_networkManager, &QNetworkAccessManager::sslErrors, this,
          &QtLicenseClient::onSslErrors);
}

QtLicenseClient::~QtLicenseClient() {}

void QtLicenseClient::setServerUrl(const QString& baseUrl) {
  m_serverUrl = baseUrl;
}

void QtLicenseClient::requestLicense(const QString& machineCode,
                                     const QString& userInfo) {
  QJsonObject json;
  json["machine_code"] = machineCode;
  json["user_info"] = userInfo;
  json["action"] = "request";

  sendPostRequest("/license/request", json, "REQUEST_LICENSE");
}

void QtLicenseClient::verifyLicenseOnline(const QString& machineCode,
                                          const QString& licenseKey) {
  QJsonObject json;
  json["machine_code"] = machineCode;
  json["license_key"] = licenseKey;
  json["action"] = "verify";

  sendPostRequest("/license/verify", json, "VERIFY_LICENSE");
}

void QtLicenseClient::getLicenseInfo(const QString& machineCode) {
  QJsonObject json;
  json["machine_code"] = machineCode;
  json["action"] = "info";

  sendPostRequest("/license/info", json, "GET_INFO");
}

void QtLicenseClient::sendPostRequest(const QString& endpoint,
                                      const QJsonObject& data,
                                      const QString& requestType) {
  // 构建完整 URL
  QString url = m_serverUrl + endpoint;
  QNetworkRequest request(url);

  // 设置请求头
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  request.setHeader(QNetworkRequest::UserAgentHeader, "QtLicenseClient/1.0");

  // SSL 配置（生产环境应该启用严格验证）
  QSslConfiguration sslConfig = request.sslConfiguration();
  sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);  // 验证服务器证书
  request.setSslConfiguration(sslConfig);

  // 将 JSON 数据转换为字节数组
  QJsonDocument doc(data);
  QByteArray jsonData = doc.toJson();

  // 发送 POST 请求，并保存请求类型
  QNetworkReply* reply = m_networkManager->post(request, jsonData);
  reply->setProperty("requestType", requestType);  // 用于区分不同类型的请求

  qDebug() << "Sending request to:" << url;
  qDebug() << "Request data:" << QString::fromUtf8(jsonData);
}

void QtLicenseClient::onRequestFinished(QNetworkReply* reply) {
  // 确保 reply 会被自动删除
  reply->deleteLater();

  // 检查网络错误
  if (reply->error() != QNetworkReply::NoError) {
    QString errorMsg = QString("Network error: %1").arg(reply->errorString());
    emit networkError(errorMsg);
    qWarning() << errorMsg;
    return;
  }

  // 读取响应数据
  QByteArray responseData = reply->readAll();
  qDebug() << "Response received:" << QString::fromUtf8(responseData);

  // 解析 JSON 响应
  QJsonDocument doc = QJsonDocument::fromJson(responseData);
  if (doc.isNull() || !doc.isObject()) {
    emit networkError("Invalid JSON response");
    return;
  }

  QJsonObject response = doc.object();
  QString requestType = reply->property("requestType").toString();

  // 根据请求类型处理不同的响应
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

void QtLicenseClient::onSslErrors(QNetworkReply* reply,
                                  const QList<QSslError>& errors) {
  qWarning() << "SSL Errors occurred:";
  for (const QSslError& error : errors) {
    qWarning() << "  -" << error.errorString();
  }

  // 生产环境不应该忽略 SSL 错误！
  // 这里仅用于开发测试（如使用自签名证书）
  // reply->ignoreSslErrors();  // 取消注释以忽略 SSL 错误（仅开发环境）

  emit networkError("SSL certificate verification failed");
}
