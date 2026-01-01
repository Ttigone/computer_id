// computer_id.cpp : Windows 机器码授权系统
// 功能：
//   1. 获取机器唯一标识码（基于CPU、主板、硬盘信息）
//   2. 验证软件授权许可证
//   3. 服务端生成许可证文件

#include <iostream>
#include <string>

#include "license_generator.h"
#include "win_product.h"

void PrintUsage() {
  printf("======================================\n");
  printf("  Windows 机器码授权系统\n");
  printf("======================================\n\n");
  printf("使用方式:\n");
  printf("  1. 客户端模式（获取机器码）:\n");
  printf("     computer_id.exe --get-code\n\n");
  printf("  2. 客户端模式（验证授权）:\n");
  printf("     computer_id.exe --verify\n\n");
  printf("  3. 服务端模式（生成许可证）:\n");
  printf("     computer_id.exe --generate <机器码>\n\n");
}

int main(int argc, char* argv[]) {
  // 如果没有参数，显示使用说明
  if (argc < 2) {
    PrintUsage();
    return 0;
  }

  std::string command = argv[1];

  // ========================================
  // 客户端：获取机器码
  // ========================================
  if (command == "--get-code") {
    printf("======================================\n");
    printf("  获取机器码\n");
    printf("======================================\n\n");

    LicenseManager licMgr;
    std::string machineCode = licMgr.GetMachineCode();

    if (machineCode.empty()) {
      printf("[错误] 无法获取机器码\n");
      return 1;
    }

    printf("硬件信息:\n");
    printf("  CPU ID:       %s\n", GetCpuId().c_str());
    printf("  主板序列号:   %s\n", GetMotherboardSerial().c_str());
    printf("  硬盘序列号:   %s\n\n", GetDiskSerial().c_str());

    printf("机器码: %s\n\n", machineCode.c_str());
    printf("请将以上机器码发送给服务端进行授权申请。\n");

    return 0;
  }

  // ========================================
  // 客户端：验证授权
  // ========================================
  else if (command == "--verify") {
    printf("======================================\n");
    printf("  验证授权\n");
    printf("======================================\n\n");

    LicenseManager licMgr;

    printf("当前机器码: %s\n", licMgr.GetMachineCode().c_str());
    printf("正在验证许可证...\n\n");

    if (licMgr.VerifyLicense("license.dat")) {
      printf("[成功] ✓ 授权验证通过！软件可以正常使用。\n");
      return 0;
    } else {
      printf("[失败] ✗ 授权验证失败！\n");
      printf("可能的原因:\n");
      printf("  1. 许可证文件 license.dat 不存在\n");
      printf("  2. 许可证文件已损坏\n");
      printf("  3. 许可证与当前机器不匹配\n\n");
      printf("请联系服务端获取有效的许可证文件。\n");
      return 1;
    }
  }

  // ========================================
  // 服务端：生成许可证
  // ========================================
  else if (command == "--generate") {
    if (argc < 3) {
      printf("[错误] 请提供客户端的机器码\n");
      printf("用法: computer_id.exe --generate <机器码>\n");
      return 1;
    }

    std::string clientMachineCode = argv[2];

    printf("======================================\n");
    printf("  服务端 - 生成许可证\n");
    printf("======================================\n\n");
    printf("客户端机器码: %s\n\n", clientMachineCode.c_str());

    GenerateLicenseForClient(clientMachineCode, "license.dat");

    return 0;
  }

  // ========================================
  // 实际软件中的授权验证流程示例
  // ========================================
  else if (command == "--demo") {
    printf("======================================\n");
    printf("  授权验证演示（集成到实际软件）\n");
    printf("======================================\n\n");

    // 在你的实际软件启动时，添加这段代码
    LicenseManager licMgr;

    if (!licMgr.VerifyLicense("license.dat")) {
      printf("[授权失败] 软件未授权，无法使用！\n\n");
      printf("您的机器码是: %s\n", licMgr.GetMachineCode().c_str());
      printf("请联系服务端获取授权。\n");
      return 1;
    }

    printf("[授权成功] 软件已授权，正常启动...\n\n");

    // 这里放你的实际软件逻辑
    printf("=== 软件主要功能运行中 ===\n");
    printf("...\n");

    return 0;
  }

  // 无效的命令
  else {
    printf("[错误] 未知命令: %s\n\n", command.c_str());
    PrintUsage();
    return 1;
  }
}
