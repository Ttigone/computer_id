# Visual Studio CMake 配置指南

## 问题描述

CMake 错误：`Target "LicenseManager" links to: Qt::Core but the target was not found`

**原因**：CMake 无法找到 Qt 的安装路径。

## 解决方案

### 方法 1：修改 CMakeSettings.json（推荐）

已创建 [CMakeSettings.json](CMakeSettings.json)，请根据你的 Qt 安装路径修改：

```json
{
  "variables": [
    {
      "name": "CMAKE_PREFIX_PATH",
      "value": "C:/Qt/6.8.1/msvc2022_64",  // 改为你的 Qt 路径
      "type": "STRING"
    }
  ]
}
```

### 方法 2：查找你的 Qt 安装路径

1. 打开 Qt 安装目录（通常在 `C:/Qt` 或 `D:/Qt`）
2. 找到对应编译器版本的路径，例如：
   - `C:/Qt/6.8.1/msvc2022_64`（Qt 6.8.1 + Visual Studio 2022）
   - `C:/Qt/6.5.0/msvc2019_64`（Qt 6.5.0 + Visual Studio 2019）
   - `C:/Qt/5.15.2/msvc2019_64`（Qt 5.15.2 + Visual Studio 2019）

3. 将路径更新到 `CMakeSettings.json` 的 `CMAKE_PREFIX_PATH` 中

### 方法 3：设置环境变量

```cmd
:: 临时设置（当前 CMD 窗口有效）
set CMAKE_PREFIX_PATH=C:/Qt/6.8.1/msvc2022_64

:: 永久设置（需要管理员权限）
setx CMAKE_PREFIX_PATH "C:/Qt/6.8.1/msvc2022_64"
```

### 方法 4：在 Visual Studio 中配置

1. 打开 Visual Studio
2. 菜单：**项目** → **CMake 设置**
3. 找到 **CMake 变量和缓存**
4. 添加新变量：
   - 名称：`CMAKE_PREFIX_PATH`
   - 值：`C:/Qt/6.8.1/msvc2022_64`
   - 类型：`STRING`

## 验证配置

### 1. 检查 Qt 安装

```cmd
dir C:\Qt
```

输出示例：
```
C:\Qt\6.8.1
C:\Qt\6.5.0
C:\Qt\5.15.2
```

### 2. 检查编译器版本

```cmd
dir C:\Qt\6.8.1
```

输出示例：
```
msvc2022_64    <- Visual Studio 2022 (64位)
msvc2019_64    <- Visual Studio 2019 (64位)
mingw_64       <- MinGW (64位)
```

### 3. 重新生成 CMake 缓存

在 Visual Studio 中：
1. 右键点击 `CMakeLists.txt`
2. 选择 **删除缓存并重新配置**
3. 查看输出窗口，应该看到：
   ```
   [CMake] Found Qt at: C:/Qt/6.8.1/msvc2022_64
   [CMake] -- Found Qt6: 6.8.1
   ```

## 如果没有安装 Qt

### 下载并安装 Qt

1. 访问官网：https://www.qt.io/download-qt-installer
2. 下载 **Qt Online Installer**
3. 安装时选择：
   - **Qt 6.8.1** 或更高版本
   - **MSVC 2022 64-bit**（对应你的 Visual Studio 版本）
   - **Qt Creator**（可选）

4. 安装路径建议：`C:\Qt`

### 最小安装组件

- Qt
  - Qt 6.8.1
    - MSVC 2022 64-bit ✅
    - Sources（可选）
    - Qt Debug Information Files（可选）
- Developer and Designer Tools
  - Qt Creator（推荐）
  - CMake（可选，VS 2022 已包含）

## 常见问题

### Q1: 错误 "AUTOGEN: No valid Qt version found"

**原因**：CMAKE_PREFIX_PATH 未设置或路径错误。

**解决**：
1. 检查 `CMakeSettings.json` 中的 `CMAKE_PREFIX_PATH`
2. 确认路径存在：`dir C:\Qt\6.8.1\msvc2022_64\lib\cmake\Qt6`

### Q2: 错误 "Qt6_DIR not found"

**原因**：Qt 版本不匹配。

**解决**：
1. 检查已安装的 Qt 版本：`dir C:\Qt`
2. 修改 `CMakeSettings.json` 为实际路径
3. 或在 `CMakeLists.txt` 中启用 Qt5：
   ```cmake
   find_package(Qt5 5.15 COMPONENTS Core Widgets)
   ```

### Q3: 多个 Qt 版本冲突

**原因**：系统中有多个 Qt 版本。

**解决**：
```json
{
  "name": "CMAKE_PREFIX_PATH",
  "value": "C:/Qt/6.8.1/msvc2022_64",  // 只指定一个版本
  "type": "STRING"
}
```

### Q4: 编译器版本不匹配

**错误信息**：
```
error LNK2019: unresolved external symbol ...
```

**原因**：Qt 编译器与 Visual Studio 不匹配。

**解决**：
- VS 2022 → 使用 `msvc2022_64`
- VS 2019 → 使用 `msvc2019_64`
- VS 2017 → 使用 `msvc2017_64`

## 完整配置示例

### CMakeSettings.json（Visual Studio）

```json
{
  "configurations": [
    {
      "name": "x64-Debug",
      "generator": "Ninja",
      "configurationType": "Debug",
      "inheritEnvironments": [ "msvc_x64_x64" ],
      "variables": [
        {
          "name": "CMAKE_TOOLCHAIN_FILE",
          "value": "G:/0_self_develop_project/VS_Project/vcpkg/scripts/buildsystems/vcpkg.cmake",
          "type": "FILEPATH"
        },
        {
          "name": "CMAKE_PREFIX_PATH",
          "value": "C:/Qt/6.8.1/msvc2022_64",
          "type": "STRING"
        }
      ]
    }
  ]
}
```

### CMakePresets.json（跨平台）

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "windows-debug",
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/out/build/${presetName}",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Debug",
        "CMAKE_TOOLCHAIN_FILE": "G:/0_self_develop_project/VS_Project/vcpkg/scripts/buildsystems/vcpkg.cmake",
        "CMAKE_PREFIX_PATH": "C:/Qt/6.8.1/msvc2022_64"
      }
    }
  ]
}
```

## 下一步

1. ✅ 确认 Qt 安装路径
2. ✅ 修改 `CMakeSettings.json` 中的 `CMAKE_PREFIX_PATH`
3. ✅ 在 Visual Studio 中删除 CMake 缓存
4. ✅ 重新配置 CMake
5. ✅ 编译项目

## 参考资料

- [Qt 官方文档 - CMake 手册](https://doc.qt.io/qt-6/cmake-manual.html)
- [Visual Studio CMake 项目](https://docs.microsoft.com/zh-cn/cpp/build/cmake-projects-in-visual-studio)
- [vcpkg + Qt 集成](https://github.com/microsoft/vcpkg/blob/master/docs/users/buildsystems/cmake-integration.md)
