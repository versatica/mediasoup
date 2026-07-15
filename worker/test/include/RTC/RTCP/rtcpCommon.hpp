#ifndef MS_TEST_RTC_RTCP_COMMON_HPP
#define MS_TEST_RTC_RTCP_COMMON_HPP

#include "common.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/NEW_RTCP/packet/Packet.hpp"
#include "Utils.hpp"
#include "test/include/testHelpers.hpp"
#include <catch2/catch_test_macros.hpp>

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
#define CHECK_RTCP_COMPOUND_PACKET(                                                                \
  /*const RTC::RTCP::CompoundPacket**/ compoundPacket,                                             \
  /*const uint8_t**/ buffer,                                                                       \
  /*size_t*/ bufferLength,                                                                         \
  /*size_t*/ length,                                                                               \
  /*size_t*/ packetsCount)                                                                         \
	do                                                                                               \
	{                                                                                                \
		REQUIRE(RTC::NEW_RTCP::Packet::IsRtcp(buffer, length) == true);                                \
		REQUIRE(compoundPacket);                                                                       \
		REQUIRE(compoundPacket->GetBuffer() != nullptr);                                               \
		REQUIRE(compoundPacket->GetBuffer() == buffer);                                                \
		REQUIRE(compoundPacket->GetBufferLength() != 0);                                               \
		REQUIRE(compoundPacket->GetBufferLength() == bufferLength);                                    \
		REQUIRE(compoundPacket->GetLength() != 0);                                                     \
		REQUIRE(compoundPacket->GetLength() == length);                                                \
		REQUIRE(                                                                                       \
		  compoundPacket->GetAvailableLength() ==                                                      \
		  compoundPacket->GetBufferLength() - compoundPacket->GetLength());                            \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(compoundPacket->GetLength()) == true);                   \
		REQUIRE(compoundPacket->GetPacketsCount() == packetsCount);                                    \
		REQUIRE_THROWS_AS(                                                                             \
		  const_cast<RTC::NEW_RTCP::CompoundPacket*>(compoundPacket)                                   \
		    ->Serialize(rtcpCommon::ThrowBuffer, length - 1),                                          \
		  MediaSoupError);                                                                             \
		REQUIRE_THROWS_AS(compoundPacket->Clone(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError); \
	} while (false)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_RTCP_PACKET(                                                                         \
  /*RTC::RTCP::Packet**/ packet,                                                                   \
  /*const uint8_t**/ buffer,                                                                       \
  /*size_t*/ bufferLength,                                                                         \
  /*size_t*/ length,                                                                               \
  /*RTC::RTCP::Packet::PacketType*/ packetType,                                                    \
  /*bool*/ unknownType)                                                                            \
	do                                                                                               \
	{                                                                                                \
		if (buffer)                                                                                    \
		{                                                                                              \
			REQUIRE(RTC::NEW_RTCP::Packet::IsRtcp(buffer, length) == true);                              \
		}                                                                                              \
		REQUIRE(packet);                                                                               \
		REQUIRE(packet->GetBuffer() != nullptr);                                                       \
		if (buffer)                                                                                    \
		{                                                                                              \
			REQUIRE(packet->GetBuffer() == buffer);                                                      \
		}                                                                                              \
		REQUIRE(packet->GetBufferLength() != 0);                                                       \
		REQUIRE(packet->GetBufferLength() == bufferLength);                                            \
		REQUIRE(packet->GetLength() != 0);                                                             \
		REQUIRE(packet->GetLength() == length);                                                        \
		REQUIRE(packet->GetAvailableLength() == packet->GetBufferLength() - packet->GetLength());      \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);                           \
		REQUIRE(static_cast<unsigned>(packet->GetVersion()) == 2);                                     \
		REQUIRE(packet->GetType() == packetType);                                                      \
		REQUIRE(packet->HasUnknownType() == unknownType);                                              \
		REQUIRE(packet->HasPadding() == false);                                                        \
		REQUIRE_THROWS_AS(packet->Serialize(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);     \
		REQUIRE_THROWS_AS(packet->Clone(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);         \
	} while (false)

#endif
