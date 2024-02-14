#pragma once

#include <string>
#include <vector>

class FileManager {
 public:
  static std::vector<char> readBinary(const char*);
  static std::vector<std::string> readFile(std::string);

  FileManager() = delete;
};