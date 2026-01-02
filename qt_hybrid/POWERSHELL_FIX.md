# PowerShell 错误快速修复指南

## 问题现象

```
'powershell.exe' is not recognized as an internal or external command
ninja: build stopped: subcommand failed
```

## 立即解决方案（3 选 1）

### 方案 1：重新配置 CMake（推荐）✅

已经修改了 `CMakeLists.txt`，现在只需：

1. **在 Visual Studio 中：**
   - 右键点击 `CMakeLists.txt`
   - 选择 **"删除缓存并重新配置"**
   - 等待配置完成
   - **生成** → **生成解决方案**

2. **结果：**
   - ✅ 不再依赖 PowerShell
   - ✅ CMake 自动复制 DLL 文件
   - ✅ 直接运行程序

---

### 方案 2：手动运行复制脚本

如果方案 1 不工作，手动复制 DLL：

```cmd
cd G:\0_self_develop_project\VS_Project\computer_id\qt_hybrid
copy_dlls.bat
```

脚本会自动：
- 复制 vcpkg DLL（OpenSSL, libcurl, zlib）
- 运行 windeployqt 复制 Qt DLL
- 放置到正确的目录

---

### 方案 3：手动复制 DLL

如果脚本不工作，完全手动操作：

#### 1. 复制 vcpkg DLL

```cmd
cd G:\0_self_develop_project\VS_Project\computer_id\qt_hybrid\out\build\x64-Debug

copy G:\0_self_develop_project\VS_Project\vcpkg\installed\x64-windows\debug\bin\libssl-3-x64.dll .
copy G:\0_self_develop_project\VS_Project\vcpkg\installed\x64-windows\debug\bin\libcrypto-3-x64.dll .
copy G:\0_self_develop_project\VS_Project\vcpkg\installed\x64-windows\debug\bin\libcurl-d.dll .
copy G:\0_self_develop_project\VS_Project\vcpkg\installed\x64-windows\debug\bin\zlib1d.dll .
```

#### 2. 复制 Qt DLL

```cmd
D:\qt\qt6\6.8.3\msvc2022_64\bin\windeployqt.exe LicenseManager.exe
```

或手动复制：
```cmd
copy D:\qt\qt6\6.8.3\msvc2022_64\bin\Qt6Cored.dll .
copy D:\qt\qt6\6.8.3\msvc2022_64\bin\Qt6Guid.dll .
copy D:\qt\qt6\6.8.3\msvc2022_64\bin\Qt6Widgetsd.dll .
```

---

## 验证修复

### 检查 DLL 是否已复制

```cmd
dir G:\0_self_develop_project\VS_Project\computer_id\qt_hybrid\out\build\x64-Debug\*.dll
```

应该看到：
```
libssl-3-x64.dll
libcrypto-3-x64.dll
libcurl-d.dll
zlib1d.dll
Qt6Cored.dll
Qt6Guid.dll
Qt6Widgetsd.dll
```

### 运行程序

```cmd
cd G:\0_self_develop_project\VS_Project\computer_id\qt_hybrid\out\build\x64-Debug
LicenseManager.exe
```

---

## 为什么会出现这个问题？

1. **vcpkg 默认行为：**
   - vcpkg 的 CMake 工具链包含 `applocal.ps1` 脚本
   - 编译后自动运行 PowerShell 复制 DLL
   - 提高开发体验，避免手动复制

2. **PowerShell 问题：**
   - 某些 Windows 系统 PowerShell 不在 PATH 中
   - 企业环境可能禁用 PowerShell 脚本执行
   - Windows Server Core 版本默认不安装 PowerShell

3. **解决方案：**
   - 禁用 vcpkg 的自动部署功能
   - 使用 CMake 原生命令复制文件
   - 或提供手动复制脚本

---

## 技术细节

### CMakeLists.txt 中的修改

```cmake
# 禁用 vcpkg 自动 DLL 复制（避免 PowerShell 依赖）
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF)
set(VCPKG_APPLOCAL_DEPS OFF)

# ... 其他配置 ...

# 自定义 DLL 复制逻辑
if(DEFINED VCPKG_INSTALLED_DIR)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
        set(VCPKG_DLL_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/debug/bin")
    else()
        set(VCPKG_DLL_DIR "${VCPKG_INSTALLED_DIR}/x64-windows/bin")
    endif()
    
    file(GLOB VCPKG_DLLS "${VCPKG_DLL_DIR}/*.dll")
    
    foreach(DLL_FILE ${VCPKG_DLLS})
        add_custom_command(TARGET LicenseManager POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${DLL_FILE}" "$<TARGET_FILE_DIR:LicenseManager>"
        )
    endforeach()
endif()
```

### 优势

- ✅ 不依赖 PowerShell
- ✅ 跨平台兼容（`${CMAKE_COMMAND} -E` 是跨平台的）
- ✅ 只在编译成功后执行
- ✅ 只复制变化的文件（`copy_if_different`）

---

## 常见问题

### Q: 为什么不直接添加 PowerShell 到 PATH？

**A:** 
- 可能没有管理员权限
- 企业环境可能禁止修改系统设置
- 使用 CMake 原生命令更可靠

### Q: CMake 自动复制失败怎么办？

**A:** 
运行手动复制脚本：
```cmd
cd qt_hybrid
copy_dlls.bat
```

### Q: 我的 Qt 路径不是 D:\qt\qt6\6.8.3\

**A:** 
修改 `copy_dlls.bat` 第 10 行：
```bat
set "QT_BIN=你的Qt路径\bin"
```

### Q: Release 版本怎么复制？

**A:** 
修改 `copy_dlls.bat`：
```bat
REM 第 7 行改为
set "BUILD_DIR=%~dp0out\build\x64-Release"

REM 第 9 行改为（不带 debug）
set "VCPKG_BIN=%VCPKG_ROOT%\installed\x64-windows\bin"
```

---

## 下次避免此问题

### 方法 1：使用已修复的 CMakeLists.txt

当前项目已经修复，以后创建新项目时：
```cmake
# 在 CMakeLists.txt 开头添加
set(X_VCPKG_APPLOCAL_DEPS_INSTALL OFF)
set(VCPKG_APPLOCAL_DEPS OFF)
```

### 方法 2：vcpkg 集成模式

不使用 CMake 工具链文件，改用 vcpkg manifest 模式。

### 方法 3：静态链接

```cmake
# 使用静态库，不需要 DLL
vcpkg install openssl:x64-windows-static
vcpkg install curl:x64-windows-static
```

---

## 总结

1. ✅ **已修复 CMakeLists.txt** - 重新配置即可
2. ✅ **提供 copy_dlls.bat** - 备用方案
3. ✅ **手动复制命令** - 终极方案

**推荐操作：**
```
Visual Studio → 右键 CMakeLists.txt → 删除缓存并重新配置 → 生成
```

搞定！🎉
