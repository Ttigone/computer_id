#include "license_main_window.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QMessageBox>
#include <QStyle>
#include <QThread>

#include "license_backend.h"

LicenseMainWindow::LicenseMainWindow(QWidget* parent) : QMainWindow(parent) {
  initUI();

  // 启动时自动获取机器码
  QTimer::singleShot(100, this, &LicenseMainWindow::onGetMachineCode);

  // 自动检查现有授权
  QTimer::singleShot(500, this, &LicenseMainWindow::onVerifyLicense);
}

LicenseMainWindow::~LicenseMainWindow() {}

void LicenseMainWindow::initUI() {
  setWindowTitle("软件授权系统 - Qt UI + 纯C++后端");
  setMinimumSize(700, 600);

  // 中心部件
  QWidget* centralWidget = new QWidget(this);
  QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);

  // 状态标签
  m_statusLabel = new QLabel("未授权", this);
  m_statusLabel->setAlignment(Qt::AlignCenter);
  m_statusLabel->setStyleSheet(
      "QLabel { "
      "font-size: 20px; "
      "font-weight: bold; "
      "padding: 15px; "
      "background-color: #f0f0f0; "
      "border-radius: 5px; "
      "color: #ff6b6b; "
      "}");
  mainLayout->addWidget(m_statusLabel);

  // 机器码组
  createMachineCodeGroup();
  mainLayout->addWidget(m_machineCodeGroup);

  // 授权组
  createLicenseGroup();
  mainLayout->addWidget(m_licenseGroup);

  // 日志组
  createLogGroup();
  mainLayout->addWidget(m_logGroup);

  // 进度条
  m_progressBar = new QProgressBar(this);
  m_progressBar->setVisible(false);
  m_progressBar->setRange(0, 0);  // 不确定进度
  mainLayout->addWidget(m_progressBar);

  setCentralWidget(centralWidget);

  appendLog("系统初始化完成");
}

void LicenseMainWindow::createMachineCodeGroup() {
  m_machineCodeGroup = new QGroupBox("机器标识码", this);
  QVBoxLayout* layout = new QVBoxLayout();

  // 机器码显示
  QHBoxLayout* codeLayout = new QHBoxLayout();
  m_machineCodeEdit = new QLineEdit(this);
  m_machineCodeEdit->setReadOnly(true);
  m_machineCodeEdit->setPlaceholderText("点击下方按钮获取机器码...");

  m_copyMachineCodeBtn = new QPushButton("复制", this);
  m_copyMachineCodeBtn->setMaximumWidth(80);
  m_copyMachineCodeBtn->setEnabled(false);

  codeLayout->addWidget(m_machineCodeEdit);
  codeLayout->addWidget(m_copyMachineCodeBtn);
  layout->addLayout(codeLayout);

  // 按钮
  m_getMachineCodeBtn = new QPushButton("获取机器码", this);
  m_getMachineCodeBtn->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));
  layout->addWidget(m_getMachineCodeBtn);

  // 说明
  QLabel* hintLabel = new QLabel("提示：将机器码发送给管理员以获取授权", this);
  hintLabel->setStyleSheet("color: #666; font-size: 11px;");
  layout->addWidget(hintLabel);

  m_machineCodeGroup->setLayout(layout);

  // 连接信号
  connect(m_getMachineCodeBtn, &QPushButton::clicked, this,
          &LicenseMainWindow::onGetMachineCode);
  connect(m_copyMachineCodeBtn, &QPushButton::clicked, this,
          &LicenseMainWindow::onCopyMachineCode);
}

void LicenseMainWindow::createLicenseGroup() {
  m_licenseGroup = new QGroupBox("授权管理", this);
  QVBoxLayout* layout = new QVBoxLayout();

  // 用户信息输入
  QHBoxLayout* userLayout = new QHBoxLayout();
  userLayout->addWidget(new QLabel("用户信息:", this));
  m_userInfoEdit = new QLineEdit(this);
  m_userInfoEdit->setPlaceholderText("邮箱或用户名（可选）");
  userLayout->addWidget(m_userInfoEdit);
  layout->addLayout(userLayout);

  // 许可证密钥
  QHBoxLayout* keyLayout = new QHBoxLayout();
  keyLayout->addWidget(new QLabel("许可证密钥:", this));
  m_licenseKeyEdit = new QLineEdit(this);
  m_licenseKeyEdit->setPlaceholderText("粘贴管理员提供的许可证...");
  keyLayout->addWidget(m_licenseKeyEdit);
  layout->addLayout(keyLayout);

  // 操作按钮
  QHBoxLayout* btnLayout = new QHBoxLayout();

  m_requestLicenseBtn = new QPushButton("在线申请授权", this);
  m_requestLicenseBtn->setIcon(
      style()->standardIcon(QStyle::SP_DialogApplyButton));
  m_requestLicenseBtn->setEnabled(false);

  m_verifyLicenseBtn = new QPushButton("验证授权", this);
  m_verifyLicenseBtn->setIcon(
      style()->standardIcon(QStyle::SP_DialogYesButton));

  m_checkInfoBtn = new QPushButton("查询信息", this);
  m_checkInfoBtn->setIcon(style()->standardIcon(QStyle::SP_FileDialogInfoView));
  m_checkInfoBtn->setEnabled(false);

  btnLayout->addWidget(m_requestLicenseBtn);
  btnLayout->addWidget(m_verifyLicenseBtn);
  btnLayout->addWidget(m_checkInfoBtn);
  layout->addLayout(btnLayout);

  m_licenseGroup->setLayout(layout);

  // 连接信号
  connect(m_requestLicenseBtn, &QPushButton::clicked, this,
          &LicenseMainWindow::onRequestLicense);
  connect(m_verifyLicenseBtn, &QPushButton::clicked, this,
          &LicenseMainWindow::onVerifyLicense);
  connect(m_checkInfoBtn, &QPushButton::clicked, this,
          &LicenseMainWindow::onCheckLicenseInfo);
}

void LicenseMainWindow::createLogGroup() {
  m_logGroup = new QGroupBox("操作日志", this);
  QVBoxLayout* layout = new QVBoxLayout();

  m_logEdit = new QTextEdit(this);
  m_logEdit->setReadOnly(true);
  m_logEdit->setMaximumHeight(150);
  m_logEdit->setStyleSheet(
      "font-family: Consolas, monospace; font-size: 10px;");

  layout->addWidget(m_logEdit);
  m_logGroup->setLayout(layout);
}

// ============================================================================
// 事件处理（在新线程中调用纯C++后端）
// ============================================================================

void LicenseMainWindow::onGetMachineCode() {
  appendLog("正在获取机器码...");
  setButtonsEnabled(false);
  m_progressBar->setVisible(true);

  // 在新线程中执行（不阻塞UI）
  QThread* thread = QThread::create([this]() {
    LicenseBackend backend;
    std::string machineCode = backend.getMachineCode();

    // 回到主线程更新UI
    QMetaObject::invokeMethod(
        this,
        [this, machineCode]() {
          m_currentMachineCode = QString::fromStdString(machineCode);
          m_machineCodeEdit->setText(m_currentMachineCode);
          m_copyMachineCodeBtn->setEnabled(true);
          m_requestLicenseBtn->setEnabled(true);
          m_checkInfoBtn->setEnabled(true);

          appendLog("机器码获取成功: " + m_currentMachineCode.left(32) + "...");

          m_progressBar->setVisible(false);
          setButtonsEnabled(true);
        },
        Qt::QueuedConnection);
  });

  thread->start();
}

void LicenseMainWindow::onRequestLicense() {
  if (m_currentMachineCode.isEmpty()) {
    QMessageBox::warning(this, "错误", "请先获取机器码");
    return;
  }

  QString userInfo = m_userInfoEdit->text().trimmed();

  appendLog("正在向服务器请求授权...");
  setButtonsEnabled(false);
  m_progressBar->setVisible(true);

  QThread* thread = QThread::create([this, userInfo]() {
    LicenseBackend backend;
    backend.setServerUrl("https://yourserver.com/api");  // 修改为实际服务器地址
    backend.setAppSecret("YOUR_STRONG_SECRET_2026");

    auto result = backend.requestLicense(m_currentMachineCode.toStdString(),
                                         userInfo.toStdString());

    QMetaObject::invokeMethod(
        this,
        [this, result]() {
          m_progressBar->setVisible(false);
          setButtonsEnabled(true);

          if (result.success) {
            m_currentLicenseKey = QString::fromStdString(result.licenseKey);
            m_licenseKeyEdit->setText(m_currentLicenseKey);

            // 保存到本地文件
            LicenseBackend::saveLicenseToFile(result.licenseKey);

            updateStatus("✓ 授权成功", true);
            appendLog("授权成功！许可证已保存");
            appendLog("过期时间: " + QString::fromStdString(result.expiresAt));

            QMessageBox::information(
                this, "成功",
                "授权申请成功！\n\n"
                "许可证密钥: " +
                    m_currentLicenseKey.left(32) +
                    "...\n"
                    "过期时间: " +
                    QString::fromStdString(result.expiresAt));
          } else {
            appendLog("授权失败: " + QString::fromStdString(result.message));

            QMessageBox::warning(this, "授权失败",
                                 QString::fromStdString(result.message));
          }
        },
        Qt::QueuedConnection);
  });

  thread->start();
}

void LicenseMainWindow::onVerifyLicense() {
  appendLog("正在验证授权...");
  setButtonsEnabled(false);
  m_progressBar->setVisible(true);

  QThread* thread = QThread::create([this]() {
    LicenseBackend backend;
    backend.setServerUrl("https://yourserver.com/api");
    backend.setAppSecret("YOUR_STRONG_SECRET_2026");

    // 获取机器码
    std::string machineCode = backend.getMachineCode();

    // 从本地加载许可证
    std::string licenseKey = LicenseBackend::loadLicenseFromFile();

    if (licenseKey.empty()) {
      QMetaObject::invokeMethod(
          this,
          [this]() {
            m_progressBar->setVisible(false);
            setButtonsEnabled(true);
            updateStatus("✗ 未授权", false);
            appendLog("本地未找到许可证文件");
          },
          Qt::QueuedConnection);
      return;
    }

    // 验证授权
    auto result = backend.verifyLicense(machineCode, licenseKey);

    QMetaObject::invokeMethod(
        this,
        [this, result, licenseKey]() {
          m_progressBar->setVisible(false);
          setButtonsEnabled(true);

          if (result.valid) {
            updateStatus("✓ 已授权", true);
            m_licenseKeyEdit->setText(QString::fromStdString(licenseKey));
            appendLog("授权验证成功");
            appendLog("过期时间: " + QString::fromStdString(result.expiresAt));
          } else {
            updateStatus("✗ 授权失败", false);
            appendLog("授权验证失败: " +
                      QString::fromStdString(result.message));

            QMessageBox::warning(this, "授权无效",
                                 QString::fromStdString(result.message) +
                                     "\n\n"
                                     "请联系管理员获取有效的授权");
          }
        },
        Qt::QueuedConnection);
  });

  thread->start();
}

void LicenseMainWindow::onCheckLicenseInfo() {
  if (m_currentMachineCode.isEmpty()) {
    QMessageBox::warning(this, "错误", "请先获取机器码");
    return;
  }

  appendLog("正在查询许可证信息...");
  setButtonsEnabled(false);
  m_progressBar->setVisible(true);

  QThread* thread = QThread::create([this]() {
    LicenseBackend backend;
    backend.setServerUrl("https://yourserver.com/api");
    backend.setAppSecret("YOUR_STRONG_SECRET_2026");

    auto result = backend.getLicenseInfo(m_currentMachineCode.toStdString());

    QMetaObject::invokeMethod(
        this,
        [this, result]() {
          m_progressBar->setVisible(false);
          setButtonsEnabled(true);

          if (result.success) {
            QString info =
                QString(
                    "许可证信息:\n\n"
                    "状态: %1\n"
                    "用户: %2\n"
                    "创建时间: %3\n"
                    "过期时间: %4\n"
                    "最后验证: %5")
                    .arg(QString::fromStdString(result.status))
                    .arg(QString::fromStdString(result.userInfo))
                    .arg(QString::fromStdString(result.createdAt))
                    .arg(QString::fromStdString(result.expiresAt))
                    .arg(QString::fromStdString(result.lastVerified));

            appendLog("许可证信息查询成功");
            QMessageBox::information(this, "许可证信息", info);
          } else {
            appendLog("查询失败: 许可证不存在");
            QMessageBox::warning(this, "查询失败", "许可证信息不存在");
          }
        },
        Qt::QueuedConnection);
  });

  thread->start();
}

void LicenseMainWindow::onCopyMachineCode() {
  if (!m_currentMachineCode.isEmpty()) {
    QApplication::clipboard()->setText(m_currentMachineCode);
    appendLog("机器码已复制到剪贴板");

    // 临时改变按钮文字
    m_copyMachineCodeBtn->setText("已复制");
    QTimer::singleShot(2000, this,
                       [this]() { m_copyMachineCodeBtn->setText("复制"); });
  }
}

// ============================================================================
// UI 辅助方法
// ============================================================================

void LicenseMainWindow::updateStatus(const QString& status, bool isSuccess) {
  m_statusLabel->setText(status);

  if (isSuccess) {
    m_statusLabel->setStyleSheet(
        "QLabel { "
        "font-size: 20px; font-weight: bold; padding: 15px; "
        "background-color: #d4edda; border-radius: 5px; "
        "color: #155724; "
        "}");
  } else {
    m_statusLabel->setStyleSheet(
        "QLabel { "
        "font-size: 20px; font-weight: bold; padding: 15px; "
        "background-color: #f8d7da; border-radius: 5px; "
        "color: #721c24; "
        "}");
  }
}

void LicenseMainWindow::appendLog(const QString& message) {
  QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  m_logEdit->append(QString("[%1] %2").arg(timestamp).arg(message));
}

void LicenseMainWindow::setButtonsEnabled(bool enabled) {
  m_getMachineCodeBtn->setEnabled(enabled);
  m_requestLicenseBtn->setEnabled(enabled && !m_currentMachineCode.isEmpty());
  m_verifyLicenseBtn->setEnabled(enabled);
  m_checkInfoBtn->setEnabled(enabled && !m_currentMachineCode.isEmpty());
}
