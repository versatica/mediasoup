#ifndef MS_TEST_HELPERS_HPP
#define MS_TEST_HELPERS_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"

namespace helpers
{
	bool readBinaryFile(const char* file, uint8_t* buffer, size_t* len);

	bool addToBuffer(uint8_t* buf, int* size, uint8_t* data, size_t len);

	bool readPayloadData(const char* file, int pos, int bytes, uint8_t* payload);

	bool writeRtpPacket(
	  const char* file,
	  uint8_t nalType,
	  size_t nalLength,
	  int32_t sid,
	  int32_t tid,
	  int32_t isIdr,
	  int32_t firstSliceId,
	  int32_t lastSliceId,
	  uint8_t* payload,
	  uint8_t* buf,
	  size_t* len);

	bool areBuffersEqual(const uint8_t* data1, size_t size1, const uint8_t* data2, size_t size2);
	std::unique_ptr<RTC::RtpPacket> CreateRtpPacket(uint8_t* payload, size_t len);
} // namespace helpers

#endif
