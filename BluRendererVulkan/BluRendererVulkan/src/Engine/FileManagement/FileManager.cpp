#include "FileManager.h"

#include <fstream>
#include <iostream>
#include <mutex>

std::vector<char> FileManager::readBinary(const char* filePath) {
  static std::mutex mutex;
  std::lock_guard<std::mutex> lock(mutex);
  std::ifstream file(filePath, std::ios::in | std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("unable to open file");
  }

  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();

  return buffer;
}

template <typename T>
void FileManager::readStructFromFile(std::string& filePath, T& data) {
  std::ifstream in;
  in.open(filePath, std::ios::binary);
  in.read(reinterpret_cast<char*>(&data), sizeof(T));
  in.close();
}

template <typename T>
void FileManager::writeStructFromFile(std::string& file_name, T& data) {
  std::ofstream out;
  out.open(file_name, std::ios::binary);
  out.write(reinterpret_cast<char*>(&data), sizeof(T));
  out.close();
}