QT       += core gui widgets
TARGET = LicenseManager
TEMPLATE = app

CONFIG += c++17

# 定义项目源文件和头文件
SOURCES += \
    main.cpp \
    license_main_window.cpp \
    license_backend.cpp \
    ../computer_id/win_product.cpp \
    ../computer_id/secure_transport_cpp.cpp \
    ../computer_id/http_client_cpp.cpp

HEADERS += \
    license_main_window.h \
    license_backend.h \
    ../computer_id/win_product.h \
    ../computer_id/secure_transport_cpp.h \
    ../computer_id/http_client_cpp.h

# OpenSSL 配置（通过 vcpkg 安装）
# 假设使用 x64-windows 架构
VCPKG_ROOT = $$(VCPKG_ROOT)
isEmpty(VCPKG_ROOT) {
    VCPKG_ROOT = C:/vcpkg
}

INCLUDEPATH += $$VCPKG_ROOT/installed/x64-windows/include
LIBS += -L$$VCPKG_ROOT/installed/x64-windows/lib

# OpenSSL 库
LIBS += -lssl -lcrypto

# libcurl 库
LIBS += -lcurl

# Windows 系统库
LIBS += -lws2_32 -lcrypt32 -lwbemuuid -lole32 -loleaut32 -ladvapi32

# 编译选项
win32 {
    DEFINES += _CRT_SECURE_NO_WARNINGS
    DEFINES += CURL_STATICLIB  # 如果使用静态链接的 libcurl
}

# 输出目录
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/../build/debug
    OBJECTS_DIR = $$PWD/../build/debug/obj
    MOC_DIR = $$PWD/../build/debug/moc
} else {
    DESTDIR = $$PWD/../build/release
    OBJECTS_DIR = $$PWD/../build/release/obj
    MOC_DIR = $$PWD/../build/release/moc
}
