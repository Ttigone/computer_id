#include <QApplication>

#include "license_main_window.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // 设置应用程序信息
  QApplication::setApplicationName("License Manager");
  QApplication::setApplicationVersion("1.0.0");
  QApplication::setOrganizationName("YourCompany");

  // 创建并显示主窗口
  LicenseMainWindow window;
  window.show();

  return app.exec();
}
