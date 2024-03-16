#pragma once

#include <string>
#include <vector>

class FileManager {
 public:
  static std::vector<char> readBinary(const char*);
  template<typename T>
  static void readStructFromFile(std::string&, T&);
  template <typename T>
  static void writeStructToFile(std::string&, T&);

  FileManager() = delete;
};