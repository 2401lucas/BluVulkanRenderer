#include "FileManager.h"
#include <iostream>
#include <mutex>
#include <fstream>

std::vector<char> Core::System::FileManager::readBinary(const char* filePath)
{
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

std::vector<std::string> FileManager::readFile(std::string) {
        return std::vector<std::string>();
}
