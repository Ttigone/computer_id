#include "win_product.h"

#include <Wbemidl.h>
#include <comdef.h>
#include <wincrypt.h>

#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "advapi32.lib")

// ============================================================================
// WMI 查询函数
// ============================================================================

std::string GetWmiProperty(const std::wstring& className,
                           const std::wstring& propertyName) {
  std::string result;

  HRESULT hres = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hres) && hres != RPC_E_CHANGED_MODE) {
    // 如果已经初始化过其他模式（如 STA），这里可能失败
    // 可以选择继续执行（很多情况下无影响），或记录日志
    // 这里我们选择继续（常见做法）
  }

  IWbemLocator* pLoc = nullptr;
  IWbemServices* pSvc = nullptr;
  IEnumWbemClassObject* pEnumerator = nullptr;
  IWbemClassObject* pclsObj = nullptr;

  // 1. 创建 WMI Locator
  hres = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pLoc));
  if (FAILED(hres)) {
    CoUninitialize();
    return "";
  }

  // 2. 连接到 WMI 命名空间
  hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, 0, 0,
                             nullptr, nullptr, &pSvc);
  if (FAILED(hres)) {
    pLoc->Release();
    CoUninitialize();
    return "";
  }

  // 3. 设置代理安全级别
  hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                           nullptr, EOAC_NONE);
  if (FAILED(hres)) {
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return "";
  }

  // 4. 执行查询
  std::wstring query = L"SELECT " + propertyName + L" FROM " + className;
  hres = pSvc->ExecQuery(_bstr_t(L"WQL"), _bstr_t(query.c_str()),
                         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                         nullptr, &pEnumerator);
  if (FAILED(hres)) {
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return "";
  }

  // 5. 获取第一条记录（通常 WMI 类只有一条）
  ULONG uReturn = 0;
  if (SUCCEEDED(pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn)) &&
      uReturn > 0) {
    VARIANT vtProp;
    VariantInit(&vtProp);  // 推荐初始化

    if (SUCCEEDED(pclsObj->Get(propertyName.c_str(), 0, &vtProp, nullptr, 0))) {
      if (vtProp.vt == VT_BSTR && vtProp.bstrVal != nullptr) {
        // 使用 WideCharToMultiByte 正确转换宽字符到多字节字符
        int len = WideCharToMultiByte(CP_UTF8, 0, vtProp.bstrVal, -1, nullptr,
                                      0, nullptr, nullptr);
        if (len > 0) {
          std::string temp(len - 1, 0);
          WideCharToMultiByte(CP_UTF8, 0, vtProp.bstrVal, -1, &temp[0], len,
                              nullptr, nullptr);
          result = temp;
        }
      }
      VariantClear(&vtProp);
    }

    pclsObj->Release();
  }

  // 6. 清理所有资源（按创建顺序逆序释放）
  if (pEnumerator) pEnumerator->Release();
  if (pSvc) pSvc->Release();
  if (pLoc) pLoc->Release();

  CoUninitialize();

  return result;
}

// ============================================================================
// 硬件信息获取函数
// ============================================================================

std::string GetCpuId() {
  return GetWmiProperty(L"Win32_Processor", L"ProcessorId");
}

std::string GetMotherboardSerial() {
  return GetWmiProperty(L"Win32_BaseBoard", L"SerialNumber");
}

std::string GetDiskSerial() {
  return GetWmiProperty(L"Win32_DiskDrive", L"SerialNumber");
}

// ============================================================================
// SHA256 哈希函数
// ============================================================================

std::string Sha256(const std::string& input) {
  HCRYPTPROV hProv = 0;
  HCRYPTHASH hHash = 0;
  std::string result;

  // 1. 获取加密服务提供者句柄
  if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES,
                           CRYPT_VERIFYCONTEXT)) {
    return "";
  }

  // 2. 创建哈希对象
  if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
    CryptReleaseContext(hProv, 0);
    return "";
  }

  // 3. 计算哈希
  if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(input.c_str()),
                     static_cast<DWORD>(input.length()), 0)) {
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return "";
  }

  // 4. 获取哈希值
  DWORD dwHashLen = 0;
  DWORD dwHashLenSize = sizeof(DWORD);
  if (CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&dwHashLen),
                        &dwHashLenSize, 0)) {
    std::vector<BYTE> hashData(dwHashLen);
    if (CryptGetHashParam(hHash, HP_HASHVAL, hashData.data(), &dwHashLen, 0)) {
      // 转换为十六进制字符串
      std::ostringstream oss;
      for (DWORD i = 0; i < dwHashLen; i++) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(hashData[i]);
      }
      result = oss.str();
    }
  }

  // 5. 清理资源
  CryptDestroyHash(hHash);
  CryptReleaseContext(hProv, 0);

  return result;
}

// ============================================================================
// 机器码生成函数
// ============================================================================

std::string GenerateMachineCode() {
  // 组合多个硬件信息
  std::string cpuId = GetCpuId();
  std::string motherboardSerial = GetMotherboardSerial();
  std::string diskSerial = GetDiskSerial();

  // 将所有信息拼接
  std::string combinedInfo = cpuId + "|" + motherboardSerial + "|" + diskSerial;

  // 如果所有信息都为空，使用计算机名作为备用
  if (combinedInfo == "||") {
    char computerName[256];
    DWORD size = sizeof(computerName);
    if (GetComputerNameA(computerName, &size)) {
      combinedInfo = std::string(computerName);
    }
  }

  // 计算 SHA256 哈希作为机器码
  return Sha256(combinedInfo);
}

// ============================================================================
// LicenseManager 类实现
// ============================================================================

LicenseManager::LicenseManager() { m_machineCode = GenerateMachineCode(); }

LicenseManager::~LicenseManager() {}

std::string LicenseManager::GetMachineCode() const { return m_machineCode; }

bool LicenseManager::VerifyLicense(const std::string& licenseFilePath) {
  // 读取许可证文件
  std::ifstream licenseFile(licenseFilePath, std::ios::binary);
  if (!licenseFile.is_open()) {
    return false;  // 许可证文件不存在
  }

  // 读取文件内容
  std::string licenseContent((std::istreambuf_iterator<char>(licenseFile)),
                             std::istreambuf_iterator<char>());
  licenseFile.close();

  if (licenseContent.empty()) {
    return false;
  }

  // 许可证格式：机器码的SHA256哈希（双重哈希）
  // 这样即使有人看到许可证文件，也无法直接反推出机器码
  std::string expectedLicense = Sha256(m_machineCode);

  return licenseContent == expectedLicense;
}

bool LicenseManager::GenerateLicenseFile(const std::string& machineCode,
                                         const std::string& licenseFilePath,
                                         const std::string& secretKey) {
  if (machineCode.empty()) {
    return false;
  }

  // 生成许可证内容：对机器码进行二次哈希
  // 可以加入密钥增强安全性
  std::string licenseContent = Sha256(machineCode + secretKey);

  // 写入文件
  std::ofstream licenseFile(licenseFilePath,
                            std::ios::binary | std::ios::trunc);
  if (!licenseFile.is_open()) {
    return false;
  }

  licenseFile << licenseContent;
  licenseFile.close();

  return true;
}
