#pragma once

#include <vector>

namespace Core {
	namespace System {
		class FileManager {
		public:
			static std::vector<char> readBinary(const char*);

			FileManager() = delete;
		};
	}
}