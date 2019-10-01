#ifndef MS_TEST_HELPERS_HPP
#define MS_TEST_HELPERS_HPP

#include "common.hpp"
#include <fstream>
#include <string>

namespace helpers
{
	inline bool readBinaryFile(const char* file, uint8_t* buffer, size_t* len)
	{
		std::string filePath = "test/" + std::string(file);
#ifdef _WIN32
		std::replace(filePath.begin(), filePath.end(), '/', '\\');
#endif
		std::ifstream in(filePath, std::ios::ate | std::ios::binary);

		if (!in)
			return false;

		*len = static_cast<size_t>(in.tellg()) - 1;
		in.seekg(0, std::ios::beg);
		in.read(reinterpret_cast<char*>(buffer), *len);
		in.close();

		return true;
	}
} // namespace helpers

#endif
