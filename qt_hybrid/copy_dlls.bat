@echo off
REM 复制运行时需要的 DLL 文件到可执行文件目录
REM 使用方法：在编译后运行此脚本

setlocal

REM 设置路径
set "BUILD_DIR=%~dp0out\build\x64-Debug"
set "VCPKG_ROOT=G:\0_self_develop_project\VS_Project\vcpkg"
set "VCPKG_BIN=%VCPKG_ROOT%\installed\x64-windows\debug\bin"
set "QT_BIN=D:\qt\qt6\6.8.3\msvc2022_64\bin"

echo ======================================
echo 复制依赖 DLL 文件
echo ======================================
echo.

REM 检查目标目录是否存在
if not exist "%BUILD_DIR%" (
    echo 错误：编译目录不存在：%BUILD_DIR%
    echo 请先编译项目！
    pause
    exit /b 1
)

REM 复制 vcpkg DLL
echo [1/2] 复制 vcpkg DLL...
if exist "%VCPKG_BIN%" (
    copy /Y "%VCPKG_BIN%\libssl-3-x64.dll" "%BUILD_DIR%\" >nul 2>&1
    copy /Y "%VCPKG_BIN%\libcrypto-3-x64.dll" "%BUILD_DIR%\" >nul 2>&1
    copy /Y "%VCPKG_BIN%\libcurl-d.dll" "%BUILD_DIR%\" >nul 2>&1
    copy /Y "%VCPKG_BIN%\zlib1d.dll" "%BUILD_DIR%\" >nul 2>&1
    echo   - libssl-3-x64.dll
    echo   - libcrypto-3-x64.dll
    echo   - libcurl-d.dll
    echo   - zlib1d.dll
) else (
    echo   警告：vcpkg bin 目录不存在
    echo   路径：%VCPKG_BIN%
)

echo.

REM 使用 windeployqt 复制 Qt DLL
echo [2/2] 复制 Qt DLL...
if exist "%QT_BIN%\windeployqt.exe" (
    "%QT_BIN%\windeployqt.exe" --no-compiler-runtime --no-translations "%BUILD_DIR%\LicenseManager.exe"
    echo   - 使用 windeployqt 完成
) else (
    echo   警告：windeployqt.exe 不存在
    echo   路径：%QT_BIN%\windeployqt.exe
    echo   手动复制 Qt DLL...
    
    if exist "%QT_BIN%" (
        copy /Y "%QT_BIN%\Qt6Cored.dll" "%BUILD_DIR%\" >nul 2>&1
        copy /Y "%QT_BIN%\Qt6Guid.dll" "%BUILD_DIR%\" >nul 2>&1
        copy /Y "%QT_BIN%\Qt6Widgetsd.dll" "%BUILD_DIR%\" >nul 2>&1
        echo   - Qt6Cored.dll
        echo   - Qt6Guid.dll
        echo   - Qt6Widgetsd.dll
    )
)

echo.
echo ======================================
echo DLL 复制完成！
echo ======================================
echo.
echo 现在可以运行：%BUILD_DIR%\LicenseManager.exe
echo.
pause
