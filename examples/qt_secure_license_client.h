#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QSslConfiguration>
#include <QString>

#include "secure_transport.h"

/// <summary>
/// Qt 客户端授权管理类（安全版本）
/// 通过 HTTPS 与服务端通信，使用多层加密保护机器码
/// </summary>
class QtSecureLicenseClient : public QObject {
  Q_OBJECT

 public:
  explicit QtSecureLicenseClient(QObject* parent = nullptr);
  ~QtSecureLicenseClient();

  /// <summary>
  /// 设置服务端 API 地址
  /// </summary>
  void setServerUrl(const QString& baseUrl);

  /// <summary>
  /// 设置应用密钥（必须与服务端保持一致）
  /// </summary>
  void setAppSecret(const QString& secret);

  /// <summary>
  /// 请求授权（安全版本）
  /// 机器码会被加密后传输，防止中间人窃取
  /// </summary>
  void requestLicense(const QString& machineCode, const QString& userInfo = "");

  /// <summary>
  /// 验证授权（安全版本）
  /// </summary>
  void verifyLicenseOnline(const QString& machineCode,
                           const QString& licenseKey);

  /// <summary>
  /// 获取许可证信息（安全版本）
  /// </summary>
  void getLicenseInfo(const QString& machineCode);

 signals:
  void licenseRequestFinished(bool success, const QString& licenseKey,
                              const QString& message);
  void licenseVerifyFinished(bool valid, const QString& message);
  void licenseInfoReceived(bool success, const QJsonObject& info);
  void networkError(const QString& errorString);

 private slots:
  void onRequestFinished(QNetworkReply* reply);
  void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);

 private:
  QNetworkAccessManager* m_networkManager;
  QString m_serverUrl;

  /// <summary>
  /// 发送加密的 POST 请求
  /// </summary>
  void sendSecurePostRequest(const QString& endpoint,
                             const QString& machineCode,
                             const QJsonObject& extraData,
                             const QString& requestType);
};

/// <summary>
/// 使用示例（安全版本）
/// </summary>
class SecureLicenseManagerExample : public QObject {
  Q_OBJECT

 public:
  explicit SecureLicenseManagerExample(QObject* parent = nullptr)
      : QObject(parent), m_secureClient(new QtSecureLicenseClient(this)) {
    // 配置服务器地址
    m_secureClient->setServerUrl("https://yourserver.com/api");

    // 设置应用密钥（与服务端保持一致）
    m_secureClient->setAppSecret("YOUR_STRONG_APP_SECRET_2026");

    // 连接信号
    connect(m_secureClient, &QtSecureLicenseClient::licenseRequestFinished,
            this, &SecureLicenseManagerExample::onLicenseRequestFinished);

    connect(m_secureClient, &QtSecureLicenseClient::licenseVerifyFinished, this,
            &SecureLicenseManagerExample::onLicenseVerifyFinished);
  }

  void checkLicense() {
    QString machineCode = getMachineCode();
    QString savedLicense = loadLicenseFromFile();

    if (savedLicense.isEmpty()) {
      // 请求授权（机器码会被加密传输）
      m_secureClient->requestLicense(machineCode);
    } else {
      // 验证授权
      m_secureClient->verifyLicenseOnline(machineCode, savedLicense);
    }
  }

 private slots:
  void onLicenseRequestFinished(bool success, const QString& licenseKey,
                                const QString& message) {
    if (success) {
      saveLicenseToFile(licenseKey);
      // 启动应用
    }
  }

  void onLicenseVerifyFinished(bool valid, const QString& message) {
    if (valid) {
      // 启动应用
    } else {
      // 显示授权失败
    }
  }

 private:
  QtSecureLicenseClient* m_secureClient;

  QString getMachineCode() { return ""; }
  QString loadLicenseFromFile() { return ""; }
  void saveLicenseToFile(const QString&) {}
};
