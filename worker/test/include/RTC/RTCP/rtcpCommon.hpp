#ifndef MS_TEST_RTC_RTCP_COMMON_HPP
#define MS_TEST_RTC_RTCP_COMMON_HPP

#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "Utils.hpp"
#include "test/include/testHelpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdlib> // std::malloc(), std::free()
#include <cstring> // std::memcpy()

namespace rtcpCommon
{
	// NOTE: We need to declare them here with `extern` and then define them in
	// rtcpCommon.cpp.
	// NOTE: Random size buffers because anyway we use sizeof(XxxxBuffer).
	extern thread_local uint8_t FactoryBuffer[66661];
	extern thread_local uint8_t SerializeBuffer[66662];
	extern thread_local uint8_t CloneBuffer[66663];
	extern thread_local uint8_t DataBuffer[66664];
	extern thread_local uint8_t ThrowBuffer[66665];

	void ResetBuffers();
} // namespace rtcpCommon

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_RTCP_PACKET(                                                                         \
  /*const RTC::RTCP::Packet**/ packet,                                                             \
  /*const uint8_t**/ buffer,                                                                       \
  /*size_t*/ bufferLength,                                                                         \
  /*size_t*/ length,                                                                               \
  /*RTC::RTCP::Packet::PacketType*/ packetType,                                                    \
  /*bool*/ unknownType)                                                                            \
	do                                                                                               \
	{                                                                                                \
		uint8_t* originalBuffer = static_cast<uint8_t*>(std::malloc(bufferLength));                    \
		std::memcpy(originalBuffer, buffer, bufferLength);                                             \
		REQUIRE(RTC::NEW_RTCP::Packet::IsRtcp(buffer, length) == true);                                \
		REQUIRE(packet);                                                                               \
		REQUIRE(packet->GetBuffer() != nullptr);                                                       \
		REQUIRE(packet->GetBuffer() == buffer);                                                        \
		REQUIRE(packet->GetBufferLength() != 0);                                                       \
		REQUIRE(packet->GetBufferLength() == bufferLength);                                            \
		REQUIRE(packet->GetLength() != 0);                                                             \
		REQUIRE(packet->GetLength() == length);                                                        \
		REQUIRE(packet->GetAvailableLength() == packet->GetBufferLength() - packet->GetLength());      \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);                           \
		REQUIRE(packet->GetType() == packetType);                                                      \
		REQUIRE(packet->HasUnknownType() == unknownType);                                              \
		REQUIRE_THROWS_AS(packet->Serialize(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);     \
		REQUIRE_THROWS_AS(packet->Clone(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);         \
		std::free(originalBuffer);                                                                     \
	} while (false)

#endif
