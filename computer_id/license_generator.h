#include <string>

#include "win_product.h"

/// <summary>
/// 服务端工具：生成许可证文件
/// 当客户端提供机器码后，在服务端运行此程序生成许可证文件
/// 然后将生成的 license.dat 文件发送给客户端
/// </summary>
/// <param name="machineCode">客户端提供的机器码</param>
/// <param name="outputPath">输出许可证文件路径</param>
void GenerateLicenseForClient(const std::string& machineCode,
                              const std::string& outputPath = "license.dat") {
  // 使用与 LicenseManager 相同的密钥
  const std::string SECRET_KEY =
      "DEFAULT_SECRET_KEY_2026";  // 生产环境应使用更复杂的密钥

  if (LicenseManager::GenerateLicenseFile(machineCode, outputPath,
                                          SECRET_KEY)) {
    printf("[服务端] 许可证文件已生成: %s\n", outputPath.c_str());
    printf("[服务端] 请将此文件发送给客户端\n");
  } else {
    printf("[服务端] 许可证生成失败\n");
  }
}
