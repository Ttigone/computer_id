#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QSslConfiguration>
#include <QString>

/// <summary>
/// Qt 客户端授权管理类
/// 通过 HTTPS 与服务端通信，实现机器码授权验证
/// </summary>
class QtLicenseClient : public QObject {
  Q_OBJECT

 public:
  explicit QtLicenseClient(QObject* parent = nullptr);
  ~QtLicenseClient();

  /// <summary>
  /// 设置服务端 API 地址
  /// </summary>
  /// <param name="baseUrl">服务端基础URL，如 https://yourserver.com/api</param>
  void setServerUrl(const QString& baseUrl);

  /// <summary>
  /// 请求授权 - 向服务端发送机器码，获取许可证
  /// </summary>
  /// <param name="machineCode">本机的机器码</param>
  /// <param name="userInfo">用户信息（可选，如用户名、邮箱等）</param>
  void requestLicense(const QString& machineCode, const QString& userInfo = "");

  /// <summary>
  /// 在线验证授权 - 实时向服务端验证许可证是否有效
  /// </summary>
  /// <param name="machineCode">本机的机器码</param>
  /// <param name="licenseKey">本地保存的许可证密钥</param>
  void verifyLicenseOnline(const QString& machineCode,
                           const QString& licenseKey);

  /// <summary>
  /// 获取许可证信息 - 查询许可证的详细信息（有效期、功能权限等）
  /// </summary>
  /// <param name="machineCode">本机的机器码</param>
  void getLicenseInfo(const QString& machineCode);

 signals:
  /// <summary>
  /// 请求授权完成信号
  /// </summary>
  /// <param name="success">是否成功</param>
  /// <param name="licenseKey">许可证密钥（成功时）</param>
  /// <param name="message">提示信息</param>
  void licenseRequestFinished(bool success, const QString& licenseKey,
                              const QString& message);

  /// <summary>
  /// 验证授权完成信号
  /// </summary>
  /// <param name="valid">许可证是否有效</param>
  /// <param name="message">提示信息</param>
  void licenseVerifyFinished(bool valid, const QString& message);

  /// <summary>
  /// 获取许可证信息完成信号
  /// </summary>
  /// <param name="success">是否成功</param>
  /// <param name="info">许可证信息（JSON格式）</param>
  void licenseInfoReceived(bool success, const QJsonObject& info);

  /// <summary>
  /// 网络错误信号
  /// </summary>
  /// <param name="errorString">错误描述</param>
  void networkError(const QString& errorString);

 private slots:
  void onRequestFinished(QNetworkReply* reply);
  void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);

 private:
  QNetworkAccessManager* m_networkManager;
  QString m_serverUrl;

  // 发送 POST 请求的通用方法
  void sendPostRequest(const QString& endpoint, const QJsonObject& data,
                       const QString& requestType);
};

/// <summary>
/// 使用示例：集成到 Qt 应用程序中
/// </summary>
class LicenseManagerExample : public QObject {
  Q_OBJECT

 public:
  explicit LicenseManagerExample(QObject* parent = nullptr)
      : QObject(parent), m_licenseClient(new QtLicenseClient(this)) {
    // 设置服务端地址
    m_licenseClient->setServerUrl("https://yourserver.com/api");

    // 连接信号槽
    connect(m_licenseClient, &QtLicenseClient::licenseRequestFinished, this,
            &LicenseManagerExample::onLicenseRequestFinished);

    connect(m_licenseClient, &QtLicenseClient::licenseVerifyFinished, this,
            &LicenseManagerExample::onLicenseVerifyFinished);

    connect(m_licenseClient, &QtLicenseClient::networkError, this,
            &LicenseManagerExample::onNetworkError);
  }

  // 示例1：启动时验证授权
  void checkLicenseOnStartup() {
    QString machineCode = getMachineCode();  // 调用你的 GetMachineCode() 函数
    QString savedLicense = loadLicenseFromFile();  // 从本地文件读取

    if (savedLicense.isEmpty()) {
      // 没有许可证，请求授权
      qDebug() << "No license found, requesting...";
      m_licenseClient->requestLicense(machineCode);
    } else {
      // 有许可证，验证有效性
      qDebug() << "Verifying existing license...";
      m_licenseClient->verifyLicenseOnline(machineCode, savedLicense);
    }
  }

  // 示例2：用户点击"激活"按钮
  void onActivateButtonClicked() {
    QString machineCode = getMachineCode();
    m_licenseClient->requestLicense(machineCode, "user@example.com");
  }

 private slots:
  void onLicenseRequestFinished(bool success, const QString& licenseKey,
                                const QString& message) {
    if (success) {
      qDebug() << "License obtained:" << licenseKey;
      saveLicenseToFile(licenseKey);  // 保存到本地
                                      // 显示成功消息，启动应用
    } else {
      qDebug() << "License request failed:" << message;
      // 显示错误消息
    }
  }

  void onLicenseVerifyFinished(bool valid, const QString& message) {
    if (valid) {
      qDebug() << "License is valid, starting application...";
      // 启动应用主界面
    } else {
      qDebug() << "License is invalid:" << message;
      // 显示授权失败界面
    }
  }

  void onNetworkError(const QString& errorString) {
    qDebug() << "Network error:" << errorString;
    // 网络错误时，可以使用离线验证作为备用方案
  }

 private:
  QtLicenseClient* m_licenseClient;

  QString getMachineCode() {
    // 调用你的 C++ 机器码获取函数
    // 可以通过 JNI 或直接链接的方式调用 GenerateMachineCode()
    return "example_machine_code_123456";
  }

  QString loadLicenseFromFile() {
    // 从文件加载许可证
    return "";
  }

  void saveLicenseToFile(const QString& license) {
    // 保存许可证到文件
  }
};
