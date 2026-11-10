#ifndef MS_TEST_RTC_RTCP_COMMON_HPP
#define MS_TEST_RTC_RTCP_COMMON_HPP

#include "common.hpp"
// NOTE: The macros below expand in the files that include this one, so the
// headers they need are re-exported to them.
// IWYU pragma: begin_exports
#include "MediaSoupErrors.hpp"
#include "RTC/RTCP/packet/CompoundPacket.hpp"
#include "RTC/RTCP/packet/ExtendedReport.hpp"
#include "RTC/RTCP/packet/Packet.hpp"
#include "Utils.hpp"
#include "test/include/testHelpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>
// IWYU pragma: end_exports

namespace rtcpCommon
{
	// NOTE: We need to declare them here with `extern` and then define them in
	// rtcpCommon.cpp.
	// NOTE: Random size buffers because anyway we use sizeof(XxxxBuffer).
	// NOTE: The `alignas(4)` must match the one in the definitions in
	// rtcpCommon.cpp. Without it, the compiler may assume that these arrays have
	// a larger alignment (on x86_64 an array of 16 bytes or more is assumed to be
	// aligned to 16 bytes) and access them with aligned SIMD instructions (such
	// as `movaps`), which crash since the buffers are only guaranteed to be
	// aligned to 4 bytes.
	alignas(4) extern thread_local uint8_t FactoryBuffer[66661];
	alignas(4) extern thread_local uint8_t SerializeBuffer[66662];
	alignas(4) extern thread_local uint8_t CloneBuffer[66663];
	alignas(4) extern thread_local uint8_t DataBuffer[66664];
	alignas(4) extern thread_local uint8_t ThrowBuffer[66665];

	void ResetBuffers();
} // namespace rtcpCommon

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_RTCP_COMPOUND_PACKET(                                                                  \
  /*const RTC::RTCP::CompoundPacket**/ compoundPacket,                                           \
  /*const uint8_t**/ buffer,                                                                         \
  /*size_t*/ bufferLength,                                                                           \
  /*size_t*/ length,                                                                                 \
  /*size_t*/ packetsCount)                                                                           \
	do                                                                                                 \
	{                                                                                                  \
		REQUIRE(RTC::RTCP::Packet::IsRtcp(buffer, length) == true);                                  \
		REQUIRE(compoundPacket);                                                                         \
		REQUIRE(compoundPacket->GetBuffer() != nullptr);                                                 \
		const std::vector<uint8_t> originalBuffer(                                                       \
		  compoundPacket->GetBuffer(), compoundPacket->GetBuffer() + compoundPacket->GetBufferLength()); \
		REQUIRE(compoundPacket->GetBuffer() == buffer);                                                  \
		REQUIRE(compoundPacket->GetBufferLength() != 0);                                                 \
		REQUIRE(compoundPacket->GetBufferLength() == bufferLength);                                      \
		REQUIRE(compoundPacket->GetLength() != 0);                                                       \
		REQUIRE(compoundPacket->GetLength() == length);                                                  \
		REQUIRE(                                                                                         \
		  compoundPacket->GetAvailableLength() ==                                                        \
		  compoundPacket->GetBufferLength() - compoundPacket->GetLength());                              \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(compoundPacket->GetLength()) == true);                     \
		REQUIRE(compoundPacket->GetPacketsCount() == packetsCount);                                      \
		REQUIRE_THROWS_AS(                                                                               \
		  const_cast<RTC::RTCP::CompoundPacket*>(compoundPacket)                                     \
		    ->Serialize(rtcpCommon::ThrowBuffer, length - 1),                                            \
		  MediaSoupError);                                                                               \
		REQUIRE_THROWS_AS(compoundPacket->Clone(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);   \
		REQUIRE(                                                                                         \
		  helpers::areBuffersEqual(                                                                      \
		    compoundPacket->GetBuffer(),                                                                 \
		    compoundPacket->GetBufferLength(),                                                           \
		    originalBuffer.data(),                                                                       \
		    originalBuffer.size()) == true);                                                             \
	} while (false)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_RTCP_PACKET(                                                                               \
  /*RTC::RTCP::Packet**/ packet,                                                                     \
  /*const uint8_t**/ buffer,                                                                             \
  /*size_t*/ bufferLength,                                                                               \
  /*size_t*/ length,                                                                                     \
  /*RTC::RTCP::Packet::PacketType*/ packetType,                                                      \
  /*bool*/ unknownType,                                                                                  \
  /*bool*/ hasPadding,                                                                                   \
  /*uint8_t*/ paddingLength)                                                                             \
	do                                                                                                     \
	{                                                                                                      \
		if (buffer)                                                                                          \
		{                                                                                                    \
			REQUIRE(RTC::RTCP::Packet::IsRtcp(buffer, length) == true);                                    \
		}                                                                                                    \
		REQUIRE(packet);                                                                                     \
		REQUIRE(packet->GetBuffer() != nullptr);                                                             \
		const std::vector<uint8_t> originalBuffer(                                                           \
		  packet->GetBuffer(), packet->GetBuffer() + packet->GetBufferLength());                             \
		if (buffer)                                                                                          \
		{                                                                                                    \
			REQUIRE(packet->GetBuffer() == buffer);                                                            \
		}                                                                                                    \
		REQUIRE(packet->GetBufferLength() != 0);                                                             \
		REQUIRE(packet->GetBufferLength() == bufferLength);                                                  \
		REQUIRE(packet->GetLength() != 0);                                                                   \
		REQUIRE(packet->GetLength() == length);                                                              \
		REQUIRE((static_cast<size_t>(Utils::Byte::Get2Bytes(packet->GetBuffer(), 2)) + 1) * 4 == length);    \
		REQUIRE(packet->GetAvailableLength() == packet->GetBufferLength() - packet->GetLength());            \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(packet->GetLength()) == true);                                 \
		REQUIRE(static_cast<unsigned>(packet->GetVersion()) == 2);                                           \
		REQUIRE(packet->GetType() == packetType);                                                            \
		REQUIRE(packet->HasUnknownType() == unknownType);                                                    \
		REQUIRE(packet->HasPadding() == hasPadding);                                                         \
		REQUIRE(static_cast<unsigned>(packet->GetPaddingLength()) == paddingLength);                         \
		if (!packet->HasPadding())                                                                           \
		{                                                                                                    \
			REQUIRE(static_cast<unsigned>(packet->GetPaddingLength()) == 0);                                   \
		}                                                                                                    \
		REQUIRE_THROWS_AS(packet->Serialize(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);           \
		REQUIRE_THROWS_AS(packet->Clone(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);               \
		REQUIRE(                                                                                             \
		  helpers::areBuffersEqual(                                                                          \
		    packet->GetBuffer(), packet->GetBufferLength(), originalBuffer.data(), originalBuffer.size()) == \
		  true);                                                                                             \
	} while (false)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_RTCP_EXTENDED_REPORT(                                                                  \
  /*RTC::RTCP::ExtendedReport**/ extendedReport,                                                 \
  /*const uint8_t**/ buffer,                                                                         \
  /*size_t*/ bufferLength,                                                                           \
  /*size_t*/ length,                                                                                 \
  /*RTC::RTCP::ExtendedReport::BlockType*/ blockType,                                            \
  /*bool*/ unknownType)                                                                              \
	do                                                                                                 \
	{                                                                                                  \
		REQUIRE(extendedReport);                                                                         \
		REQUIRE(extendedReport->GetBuffer() != nullptr);                                                 \
		const std::vector<uint8_t> originalBuffer(                                                       \
		  extendedReport->GetBuffer(), extendedReport->GetBuffer() + extendedReport->GetBufferLength()); \
		if (buffer)                                                                                      \
		{                                                                                                \
			REQUIRE(extendedReport->GetBuffer() == buffer);                                                \
		}                                                                                                \
		REQUIRE(extendedReport->GetBufferLength() != 0);                                                 \
		REQUIRE(extendedReport->GetBufferLength() == bufferLength);                                      \
		REQUIRE(extendedReport->GetLength() != 0);                                                       \
		REQUIRE(extendedReport->GetLength() == length);                                                  \
		REQUIRE(                                                                                         \
		  (static_cast<size_t>(Utils::Byte::Get2Bytes(extendedReport->GetBuffer(), 2)) + 1) * 4 ==       \
		  length);                                                                                       \
		REQUIRE(                                                                                         \
		  extendedReport->GetAvailableLength() ==                                                        \
		  extendedReport->GetBufferLength() - extendedReport->GetLength());                              \
		REQUIRE(Utils::Byte::IsPaddedTo4Bytes(extendedReport->GetLength()) == true);                     \
		REQUIRE(extendedReport->GetType() == blockType);                                                 \
		REQUIRE(extendedReport->HasUnknownType() == unknownType);                                        \
		REQUIRE_THROWS_AS(                                                                               \
		  extendedReport->Serialize(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);               \
		REQUIRE_THROWS_AS(extendedReport->Clone(rtcpCommon::ThrowBuffer, length - 1), MediaSoupError);   \
		REQUIRE(                                                                                         \
		  helpers::areBuffersEqual(                                                                      \
		    extendedReport->GetBuffer(),                                                                 \
		    extendedReport->GetBufferLength(),                                                           \
		    originalBuffer.data(),                                                                       \
		    originalBuffer.size()) == true);                                                             \
	} while (false)

#endif
