#ifndef MS_TEST_HELPERS_HPP
#define	MS_TEST_HELPERS_HPP

#include "common.hpp"
#include <string>
#include <fstream>

class Helpers
{
public:
	static bool ReadBinaryFile(const char* file, uint8_t* buffer, size_t* len)
	{
		std::string file_path = "test/" + std::string(file);

		std::ifstream in(file_path, std::ios::ate | std::ios::binary);
		if (!in)
			return false;

		*len = (size_t)in.tellg() - 1;
		in.seekg(0, std::ios::beg);
		in.read((char*)buffer, *len);
		in.close();

		return true;
	}
};

#endif
