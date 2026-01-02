#include "license_backend.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "../computer_id/win_product.h"

LicenseBackend::LicenseBackend() : m_client(nullptr), m_serverUrl("") {}

LicenseBackend::~LicenseBackend() {
  if (m_client) {
    delete m_client;
    m_client = nullptr;
  }
}

void LicenseBackend::setServerUrl(const std::string& url) {
  m_serverUrl = url;

  // 重新创建客户端
  if (m_client) {
    delete m_client;
  }

  if (!url.empty()) {
    m_client = new LicenseClientCpp(url);
  }
}

void LicenseBackend::setAppSecret(const std::string& secret) {
  if (m_client) {
    m_client->setAppSecret(secret);
  }
}

std::string LicenseBackend::getMachineCode() {
  try {
    return GenerateMachineCode();
  } catch (const std::exception& e) {
    std::cerr << "Error generating machine code: " << e.what() << std::endl;
    return "";
  }
}

LicenseClientCpp::LicenseResponse LicenseBackend::requestLicense(
    const std::string& machineCode, const std::string& userInfo) {
  LicenseClientCpp::LicenseResponse response;
  response.success = false;

  if (!m_client) {
    response.error = "Client not initialized. Please set server URL first.";
    return response;
  }

  if (machineCode.empty()) {
    response.error = "Machine code is empty";
    return response;
  }

  try {
    response = m_client->requestLicense(machineCode, userInfo);
  } catch (const std::exception& e) {
    response.success = false;
    response.error = std::string("Exception: ") + e.what();
  }

  return response;
}

LicenseClientCpp::VerifyResponse LicenseBackend::verifyLicense(
    const std::string& machineCode, const std::string& licenseKey) {
  LicenseClientCpp::VerifyResponse response;
  response.valid = false;

  if (!m_client) {
    response.error = "Client not initialized. Please set server URL first.";
    return response;
  }

  if (machineCode.empty()) {
    response.error = "Machine code is empty";
    return response;
  }

  if (licenseKey.empty()) {
    response.error = "License key is empty";
    return response;
  }

  try {
    response = m_client->verifyLicense(machineCode, licenseKey);
  } catch (const std::exception& e) {
    response.valid = false;
    response.error = std::string("Exception: ") + e.what();
  }

  return response;
}

LicenseClientCpp::LicenseInfo LicenseBackend::getLicenseInfo(
    const std::string& machineCode) {
  LicenseClientCpp::LicenseInfo info;

  if (!m_client) {
    return info;
  }

  if (machineCode.empty()) {
    return info;
  }

  try {
    info = m_client->getLicenseInfo(machineCode);
  } catch (const std::exception& e) {
    std::cerr << "Error getting license info: " << e.what() << std::endl;
  }

  return info;
}

bool LicenseBackend::saveLicenseToFile(const std::string& licenseKey,
                                       const std::string& filePath) {
  try {
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
      return false;
    }

    file << licenseKey;
    file.close();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error saving license: " << e.what() << std::endl;
    return false;
  }
}

std::string LicenseBackend::loadLicenseFromFile(const std::string& filePath) {
  try {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
      return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return buffer.str();
  } catch (const std::exception& e) {
    std::cerr << "Error loading license: " << e.what() << std::endl;
    return "";
  }
}

bool LicenseBackend::deleteLicenseFile(const std::string& filePath) {
  try {
    return std::remove(filePath.c_str()) == 0;
  } catch (const std::exception& e) {
    std::cerr << "Error deleting license: " << e.what() << std::endl;
    return false;
  }
}
