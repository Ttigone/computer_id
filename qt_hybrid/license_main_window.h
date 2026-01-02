#pragma once

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QString>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

/// <summary>
/// Qt UI 主窗口
/// 只负责界面显示，业务逻辑使用纯 C++ 实现
/// </summary>
class LicenseMainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit LicenseMainWindow(QWidget* parent = nullptr);
  ~LicenseMainWindow();

 private slots:
  // UI 事件处理
  void onGetMachineCode();
  void onRequestLicense();
  void onVerifyLicense();
  void onCheckLicenseInfo();
  void onCopyMachineCode();

 private:
  // 初始化UI
  void initUI();
  void createMachineCodeGroup();
  void createLicenseGroup();
  void createLogGroup();

  // 更新UI状态
  void updateStatus(const QString& status, bool isSuccess);
  void appendLog(const QString& message);
  void setButtonsEnabled(bool enabled);

  // UI 组件
  QGroupBox* m_machineCodeGroup;
  QGroupBox* m_licenseGroup;
  QGroupBox* m_logGroup;

  QLabel* m_statusLabel;
  QLabel* m_machineCodeLabel;
  QLineEdit* m_machineCodeEdit;
  QLineEdit* m_userInfoEdit;
  QLineEdit* m_licenseKeyEdit;

  QPushButton* m_getMachineCodeBtn;
  QPushButton* m_copyMachineCodeBtn;
  QPushButton* m_requestLicenseBtn;
  QPushButton* m_verifyLicenseBtn;
  QPushButton* m_checkInfoBtn;

  QTextEdit* m_logEdit;
  QProgressBar* m_progressBar;

  // 数据
  QString m_currentMachineCode;
  QString m_currentLicenseKey;
};
