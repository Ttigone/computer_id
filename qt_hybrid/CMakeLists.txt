cmake_minimum_required(VERSION 3.16)
project(LicenseManager VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 自动查找 Qt 安装路径（如果 CMAKE_PREFIX_PATH 未设置）
if(NOT DEFINED CMAKE_PREFIX_PATH)
    # 常见的 Qt 安装路径
    set(QT_SEARCH_PATHS
        "C:/Qt/6.8.1/msvc2022_64"
        "C:/Qt/6.8.0/msvc2022_64"
        "C:/Qt/6.7.0/msvc2019_64"
        "C:/Qt/6.6.0/msvc2019_64"
        "C:/Qt/6.5.0/msvc2019_64"
        "C:/Qt/5.15.2/msvc2019_64"
        "D:/Qt/6.8.1/msvc2022_64"
        "D:/Qt/6.5.0/msvc2019_64"
        "$ENV{QTDIR}"
    )
    
    foreach(QT_PATH ${QT_SEARCH_PATHS})
        if(EXISTS "${QT_PATH}/lib/cmake")
            set(CMAKE_PREFIX_PATH "${QT_PATH}")
            message(STATUS "Found Qt at: ${QT_PATH}")
            break()
        endif()
    endforeach()
endif()

# 查找 Qt（优先 Qt6，回退到 Qt5）
find_package(Qt6 COMPONENTS Core Widgets QUIET)
if (NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Core Widgets QUIET)
    if (NOT Qt5_FOUND)
        message(FATAL_ERROR 
            "Qt not found! Please install Qt or set CMAKE_PREFIX_PATH.\n"
            "Example: cmake -DCMAKE_PREFIX_PATH=C:/Qt/6.8.1/msvc2022_64 ..\n"
            "Or set environment variable: set CMAKE_PREFIX_PATH=C:/Qt/6.8.1/msvc2022_64")
    endif()
endif()

# 禁用 vcpkg 的自动 DLL 复制（避免 PowerShell 依赖）
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF)
set(VCPKG_APPLOCAL_DEPS OFF)

# 查找 OpenSSL
find_package(OpenSSL REQUIRED)

# 查找 CURL
find_package(CURL REQUIRED)

# 源文件
set(PROJECT_SOURCES
    main.cpp
    license_main_window.h
    license_main_window.cpp
    license_backend.h
    license_backend.cpp
    ../computer_id/win_product.h
    ../computer_id/win_product.cpp
    ../computer_id/secure_transport_cpp.h
    ../computer_id/secure_transport_cpp.cpp
    ../computer_id/http_client_cpp.h
    ../computer_id/http_client_cpp.cpp
)

# 创建可执行文件
if(Qt6_FOUND)
    qt_add_executable(LicenseManager ${PROJECT_SOURCES})
else()
    add_executable(LicenseManager ${PROJECT_SOURCES})
endif()

# 链接库
target_link_libraries(LicenseManager PRIVATE
    Qt::Core
    Qt::Widgets
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
)

# Windows 特定配置
if(WIN32)
    target_link_libraries(LicenseManager PRIVATE
        wbemuuid
        ole32
        oleaut32
        advapi32
        ws2_32
        crypt32
    )
    
    target_compile_definitions(LicenseManager PRIVATE
        _CRT_SECURE_NO_WARNINGS
        CURL_STATICLIB  # 如果使用静态链接的 libcurl
    )
    
    set_target_properties(LicenseManager PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
    
    # 复制运行时需要的 DLL 文件（避免使用 PowerShell）
    if(DEFINED VCPKG_INSTALLED_DIR)
        # vcpkg DLL 路径
        if(CMAKE_BUILD_TYPE MATCHES Debug)
            set(VCPKG_DLL_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/debug/bin")
        else()
            set(VCPKG_DLL_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/bin")
        endif()
        
        # 查找并复制 OpenSSL 和 curl DLL
        file(GLOB VCPKG_DLLS 
            "${VCPKG_DLL_DIR}/libssl*.dll"
            "${VCPKG_DLL_DIR}/libcrypto*.dll"
            "${VCPKG_DLL_DIR}/libcurl*.dll"
            "${VCPKG_DLL_DIR}/zlib*.dll"
        )
        
        foreach(DLL_FILE ${VCPKG_DLLS})
            add_custom_command(TARGET LicenseManager POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_FILE}"
                    "$<TARGET_FILE_DIR:LicenseManager>"
                COMMENT "Copying ${DLL_FILE}"
            )
        endforeach()
    endif()
    
    # 使用 Qt 的 windeployqt 工具
    if(Qt6_FOUND)
        get_target_property(QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
        get_filename_component(QT_BIN_DIR "${QMAKE_EXECUTABLE}" DIRECTORY)
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")
        
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET LicenseManager POST_BUILD
                COMMAND "${WINDEPLOYQT_EXECUTABLE}" 
                    --no-compiler-runtime
                    --no-translations
                    "$<TARGET_FILE:LicenseManager>"
                COMMENT "Running windeployqt..."
            )
        endif()
    endif()
endif()

# 安装配置
install(TARGETS LicenseManager
    RUNTIME DESTINATION bin
)
