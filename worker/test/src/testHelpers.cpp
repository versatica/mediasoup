#define MS_CLASS "TEST::HELPERS"

#include "testHelpers.hpp" // in worker/test/include/
#include "Logger.hpp"
#include <cstring> // std::memcmp()
#include <fstream>
#include <string>

namespace helpers
{
	bool ReadBinaryFile(const char* file, uint8_t* buffer, size_t* len)
	{
		MS_TRACE();

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

	bool AddToBuffer(uint8_t* buf, size_t* size, const uint8_t* data, size_t len)
	{
		MS_TRACE();

		static size_t bufferSize{ 65536 };

		if (*size + len > bufferSize)
		{
			return false;
		}

		size_t i{ 0 };

		if (len == 1)
		{
			buf[*size] = *data;
		}
		else
		{
			for (i = 0; i < len; ++i)
			{
				buf[*size + i] = data[i];
			}
		}

		*size += len;

		return true;
	}

	bool ReadPayloadData(const char* file, int pos, int bytes, uint8_t* payload)
	{
		MS_TRACE();

		std::string filePath = "test/" + std::string(file);

#ifdef _WIN32
		std::replace(filePath.begin(), filePath.end(), '/', '\\');
#endif

		std::ifstream in(filePath, std::ios::ate | std::ios::binary);

		if (!in)
		{
			return false;
		}

		in.seekg(pos, std::ios::beg);
		in.read(reinterpret_cast<char*>(payload), bytes);

		in.close();

		return true;
	}

	bool AreBuffersEqual(const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2)
	{
		MS_TRACE();

		if (size1 != size2)
		{
			return false;
		}

		return std::memcmp(data1, data2, size1) == 0;
	}
} // namespace helpers
