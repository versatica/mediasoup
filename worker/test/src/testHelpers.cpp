#define MS_CLASS "TEST::HELPERS"

#include "testHelpers.hpp" // in worker/test/include/
#include "Logger.hpp"
#include <cstring> // std::memcmp()
#include <fstream>
#include <string>

namespace helpers
{
	bool readBinaryFile(const char* file, uint8_t* buffer, size_t* len)
	{
		MS_TRACE();

		// NOLINTNEXTLINE(misc-const-correctness)
		std::string filePath = "test/" + std::string(file);

#ifdef _WIN32
		std::replace(filePath.begin(), filePath.end(), '/', '\\');
#endif

		std::ifstream in(filePath, std::ios::ate | std::ios::binary);

		if (!in)
		{
			return false;
		}

		*len = static_cast<size_t>(in.tellg()) - 1;

		in.seekg(0, std::ios::beg);
		in.read(reinterpret_cast<char*>(buffer), *len);
		in.close();

		return true;
	}

	bool areBuffersEqual(const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2)
	{
		MS_TRACE();

		if (size1 != size2)
		{
			return false;
		}

		return std::memcmp(data1, data2, size1) == 0;
	}
} // namespace helpers
