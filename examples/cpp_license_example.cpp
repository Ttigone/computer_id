// 纯 C++ 授权客户端使用示例
// 展示如何在 Qt UI 项目中使用纯 C++ 的网络和加密功能

#include <fstream>
#include <iostream>

#include "http_client_cpp.h"
#include "secure_transport_cpp.h"
#include "win_product.h"

// ============================================================================
// 示例 1: 基础使用（命令行程序）
// ============================================================================

void example1_BasicUsage() {
  std::cout << "=== 纯 C++ 授权示例 ===\n\n";

  // 1. 设置应用密钥
  SecureTransportCpp::setAppSecret("YOUR_STRONG_SECRET_2026");

  // 2. 创建授权客户端
  LicenseClientCpp client("https://yourserver.com/api");
  client.setAppSecret("YOUR_STRONG_SECRET_2026");

  // 3. 获取机器码
  std::string machineCode = GenerateMachineCode();
  std::cout << "机器码: " << machineCode << "\n\n";

  // 4. 请求授权
  std::cout << "正在请求授权...\n";
  auto response = client.requestLicense(machineCode, "test@example.com");

  if (response.success) {
    std::cout << "[成功] 获得许可证!\n";
    std::cout << "许可证密钥: " << response.licenseKey << "\n";
    std::cout << "过期时间: " << response.expiresAt << "\n";

    // 保存许可证到文件
    std::ofstream file("license.dat");
    file << response.licenseKey;
    file.close();
  } else {
    std::cout << "[失败] " << response.message << "\n";
  }
}

// ============================================================================
// 示例 2: 验证授权
// ============================================================================

void example2_VerifyLicense() {
  std::cout << "\n=== 验证授权 ===\n\n";

  LicenseClientCpp client("https://yourserver.com/api");
  client.setAppSecret("YOUR_STRONG_SECRET_2026");

  std::string machineCode = GenerateMachineCode();

  // 从文件读取许可证
  std::ifstream file("license.dat");
  std::string licenseKey;
  if (file.is_open()) {
    std::getline(file, licenseKey);
    file.close();
  }

  if (licenseKey.empty()) {
    std::cout << "[错误] 未找到许可证文件\n";
    return;
  }

  // 验证授权
  std::cout << "正在验证授权...\n";
  auto response = client.verifyLicense(machineCode, licenseKey);

  if (response.valid) {
    std::cout << "[成功] 授权有效\n";
    std::cout << "过期时间: " << response.expiresAt << "\n";
  } else {
    std::cout << "[失败] " << response.message << "\n";
  }
}

// ============================================================================
// 示例 3: 异步请求（不阻塞）
// ============================================================================

void example3_AsyncRequest() {
  std::cout << "\n=== 异步请求示例 ===\n\n";

  HttpClientCpp httpClient;
  httpClient.addHeader("Content-Type", "application/json");

  // 异步 GET 请求
  std::cout << "发送异步请求...\n";
  httpClient.getAsync(
      "https://api.github.com", [](const HttpClientCpp::Response& response) {
        if (response.success) {
          std::cout << "[异步回调] 请求成功!\n";
          std::cout << "状态码: " << response.statusCode << "\n";
          std::cout << "响应体: " << response.body.substr(0, 100) << "...\n";
        } else {
          std::cout << "[异步回调] 请求失败: " << response.error << "\n";
        }
      });

  std::cout << "主线程继续执行...\n";

  // 等待异步请求完成
  std::this_thread::sleep_for(std::chrono::seconds(3));
}

// ============================================================================
// 示例 4: 集成到 Qt 项目（仅使用 Qt UI）
// ============================================================================

#ifdef QT_VERSION
#include <QApplication>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QThread>
#include <QVBoxLayout>

class LicenseWindow : public QMainWindow {
  Q_OBJECT

 public:
  LicenseWindow(QWidget* parent = nullptr) : QMainWindow(parent) {
    setWindowTitle("软件授权系统（纯C++后端）");
    resize(400, 300);

    // 创建 UI（使用 Qt）
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    m_statusLabel = new QLabel("未授权", this);
    m_statusLabel->setStyleSheet("font-size: 16px; font-weight: bold;");

    QPushButton* activateBtn = new QPushButton("激活软件", this);
    QPushButton* verifyBtn = new QPushButton("验证授权", this);

    layout->addWidget(m_statusLabel);
    layout->addWidget(activateBtn);
    layout->addWidget(verifyBtn);
    layout->addStretch();

    setCentralWidget(centralWidget);

    // 连接信号（UI 部分使用 Qt）
    connect(activateBtn, &QPushButton::clicked, this,
            &LicenseWindow::onActivate);
    connect(verifyBtn, &QPushButton::clicked, this, &LicenseWindow::onVerify);

    // 启动时自动验证（使用纯 C++）
    QTimer::singleShot(0, this, &LicenseWindow::onVerify);
  }

 private slots:
  void onActivate() {
    m_statusLabel->setText("正在请求授权...");

    // 使用纯 C++ 网络模块（不依赖 Qt Network）
    std::thread([this]() {
      LicenseClientCpp client("https://yourserver.com/api");
      std::string machineCode = GenerateMachineCode();

      auto response = client.requestLicense(machineCode, "user@example.com");

      // 回到主线程更新 UI
      QMetaObject::invokeMethod(
          this,
          [this, response]() {
            if (response.success) {
              // 保存许可证
              std::ofstream file("license.dat");
              file << response.licenseKey;
              file.close();

              m_statusLabel->setText("激活成功！");
              QMessageBox::information(this, "成功", "软件已激活");
            } else {
              m_statusLabel->setText("激活失败");
              QMessageBox::warning(this, "失败",
                                   QString::fromStdString(response.message));
            }
          },
          Qt::QueuedConnection);
    }).detach();
  }

  void onVerify() {
    m_statusLabel->setText("正在验证授权...");

    std::thread([this]() {
      // 读取许可证
      std::ifstream file("license.dat");
      std::string licenseKey;
      if (file.is_open()) {
        std::getline(file, licenseKey);
        file.close();
      }

      if (licenseKey.empty()) {
        QMetaObject::invokeMethod(
            this, [this]() { m_statusLabel->setText("未找到许可证"); },
            Qt::QueuedConnection);
        return;
      }

      // 使用纯 C++ 验证（不依赖 Qt）
      LicenseClientCpp client("https://yourserver.com/api");
      std::string machineCode = GenerateMachineCode();

      auto response = client.verifyLicense(machineCode, licenseKey);

      QMetaObject::invokeMethod(
          this,
          [this, response]() {
            if (response.valid) {
              m_statusLabel->setText("✓ 授权有效");
              m_statusLabel->setStyleSheet("color: green; font-size: 16px;");
            } else {
              m_statusLabel->setText("✗ 授权无效");
              m_statusLabel->setStyleSheet("color: red; font-size: 16px;");

              QMessageBox::warning(this, "授权失败",
                                   QString::fromStdString(response.message));
            }
          },
          Qt::QueuedConnection);
    }).detach();
  }

 private:
  QLabel* m_statusLabel;
};

int example4_QtIntegration(int argc, char* argv[]) {
  QApplication app(argc, argv);

  LicenseWindow window;
  window.show();

  return app.exec();
}
#endif

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
  std::cout << "========================================\n";
  std::cout << "  纯 C++ 授权系统示例\n";
  std::cout << "  (不依赖 Qt，仅使用 OpenSSL + libcurl)\n";
  std::cout << "========================================\n\n";

  // 选择示例
  std::cout << "选择示例:\n";
  std::cout << "1. 基础使用（请求授权）\n";
  std::cout << "2. 验证授权\n";
  std::cout << "3. 异步请求\n";
#ifdef QT_VERSION
  std::cout << "4. Qt UI 集成\n";
#endif
  std::cout << "\n输入选项: ";

  int choice;
  std::cin >> choice;

  switch (choice) {
    case 1:
      example1_BasicUsage();
      break;
    case 2:
      example2_VerifyLicense();
      break;
    case 3:
      example3_AsyncRequest();
      break;
#ifdef QT_VERSION
    case 4:
      return example4_QtIntegration(argc, argv);
#endif
    default:
      std::cout << "无效选项\n";
  }

  return 0;
}
