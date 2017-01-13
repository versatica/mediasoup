#ifndef MS_TEST_HELPERS_H
#define	MS_TEST_HELPERS_H

#include "common.h"
#include <string>
#include <fstream>

class Helpers
{
public:
	static bool ReadBinaryFile(const char* file, uint8_t* buffer, size_t* len)
	{
		std::string file_path = "worker/test/" + std::string(file);

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
