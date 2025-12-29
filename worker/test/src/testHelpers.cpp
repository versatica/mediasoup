#define MS_CLASS "TEST::HELPERS"

#include "testHelpers.hpp" // in worker/test/include/
#include "Logger.hpp"
#include <cstring> // std::memcmp(), std::memset()
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

	bool WriteRtpPacket(
	  const char* file,
	  uint8_t /*nalType*/,
	  size_t nalLength,
	  int32_t sid,
	  int32_t tid,
	  int32_t isIdr,
	  int32_t firstSliceId,
	  int32_t lastSliceId,
	  uint8_t* payload,
	  uint8_t* buf,
	  size_t* len)
	{
		MS_TRACE();

		std::string filePath = "test/" + std::string(file);

#ifdef _WIN32
		std::replace(filePath.begin(), filePath.end(), '/', '\\');
#endif

		uint8_t buffer[16] = { 144, 111, 92, 65, 98, 245, 71, 218, 159, 113, 8, 226, 190, 222, 0, 1 };

		size_t packetSize = 0;
		uint8_t oneByte   = 0;

		// Write the RTP header.
		// NOLINTNEXTLINE (readability-suspicious-call-argument)
		if (!AddToBuffer(buf, &packetSize, buffer, 16))
		{
			return false;
		}

		// Write ID and length of frame marking extension if first layer then
		// length should be 0, else 1.
		oneByte = oneByte | 1 << 4;

		if (sid != -1)
		{
			oneByte = oneByte | 0x01;
		}

		if (!AddToBuffer(buf, &packetSize, &oneByte, 1))
		{
			return false;
		}

		// Write SEIDB TID bits.
		oneByte = 0;

		if (firstSliceId == 1)
		{
			oneByte = oneByte | 1 << 7;
		}

		if (lastSliceId == 1)
		{
			oneByte = oneByte | 1 << 6;
		}

		if (isIdr == 1)
		{
			oneByte = oneByte | 1 << 5;
		}

		if (tid != -1)
		{
			oneByte = oneByte | tid;
		}

		if (!AddToBuffer(buf, &packetSize, &oneByte, 1))
		{
			return false;
		}

		// Write DID QID bits.
		oneByte = 0;

		if (sid != -1)
		{
			oneByte = oneByte | sid << 6;
		}

		if (!AddToBuffer(buf, &packetSize, &oneByte, 1))
		{
			return false;
		}

		// Write TL0PICIDX.
		oneByte = 0;

		if (!AddToBuffer(buf, &packetSize, &oneByte, 1))
		{
			return false;
		}

		// Write payload.
		if (!AddToBuffer(buf, &packetSize, payload, nalLength))
		{
			return false;
		}

		*len = packetSize;

		std::FILE* f = std::fopen(filePath.c_str(), "wb");

		MS_ASSERT(f != nullptr, "cannot open file %s", filePath.c_str());

		const auto written = fwrite(buf, 1, packetSize, f);

		MS_ASSERT(
		  written == packetSize,
		  "[%zu] should be written to file [%s], but %zu bytes were written",
		  packetSize,
		  filePath.c_str(),
		  written);

		std::fclose(f);

		return true;
	}

	static uint8_t Buffer[65536] = { 0 };

	std::unique_ptr<RTC::RtpPacket> CreateRtpPacket(uint8_t* payload, size_t len)
	{
		MS_TRACE();

		// clang-format off
		const uint8_t headers[] =
		{
			0x80, 0x01, 0x00, 0x08,
			0x00, 0x00, 0x00, 0x04,
			0x00, 0x00, 0x00, 0x05
		};
		// clang-format off

		std::memcpy(Buffer, headers, sizeof(headers));
		std::memcpy(Buffer + sizeof(headers), payload, len);

		std::unique_ptr<RTC::RtpPacket> rtpPacket{ RTC::RtpPacket::Parse(Buffer, sizeof(headers)+len) };

		MS_ASSERT(rtpPacket != nullptr, "invalid packet");

		rtpPacket.reset(rtpPacket->Clone());

		return rtpPacket;
	}
} // namespace helpers
