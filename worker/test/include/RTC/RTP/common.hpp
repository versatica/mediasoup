#ifndef MS_TEST_RTC_RTP_COMMON_HPP
#define MS_TEST_RTC_RTP_COMMON_HPP

#include "common.hpp"
#include "MediaSoupErrors.hpp"          // IWYU pragma: export
#include "helpers.hpp"                  // IWYU pragma: export in worker/test/include/
#include "RTC/RTP/Packet.hpp"           // IWYU pragma: export
#include <catch2/catch_test_macros.hpp> // IWYU pragma: export

namespace RTC
{
	namespace RTP
	{
		// NOTE: We need to declare them here with `extern` and then define them in
		// common.cpp.
		extern thread_local uint8_t FactoryBuffer[66661];
		extern thread_local uint8_t SerializeBuffer[66662];
		extern thread_local uint8_t CloneBuffer[66663];
		extern thread_local uint8_t DataBuffer[66664];
		extern thread_local uint8_t ThrowBuffer[66665];

		void ResetBuffers();
	} // namespace RTP
} // namespace RTC

// clang-format off
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_RTP_PACKET( \
  /*const Packet**/ packet, \
  /*const uint8_t**/ buffer, \
  /*size_t*/ bufferLength, \
  /*size_t*/ length, \
  /*bool*/ frozen, \
  /*uint8_t*/ payloadType, \
  /*bool*/ hasMarker,                                                                    \
  /*uint16_t*/ seqNumber,                                                                    \
  /*uint32_t*/ timestamp, \
  /*uint32_t*/ ssrc,                                               \
  /*bool*/ hasCsrcs, \
  /*bool*/ hasHeaderExtension) \
	do \
	{ \
		REQUIRE(packet); \
		REQUIRE(packet->GetBuffer() != nullptr); \
		REQUIRE(packet->GetBuffer() == buffer); \
		REQUIRE(packet->GetBufferLength() != 0); \
		REQUIRE(packet->GetBufferLength() == bufferLength); \
		REQUIRE(packet->GetLength() != 0); \
		REQUIRE(packet->GetLength() == length); \
		REQUIRE(packet->IsFrozen() == frozen); \
		REQUIRE(packet->GetVersion() == 2);                                                        \
		REQUIRE(packet->GetPayloadType() == payloadType); \
		REQUIRE(packet->HasMarker() == hasMarker);                                      \
		REQUIRE(packet->GetSequenceNumber() == seqNumber);                                      \
		REQUIRE(packet->GetTimestamp() == timestamp); \
		REQUIRE(packet->GetSsrc() == ssrc);                           \
		REQUIRE(packet->HasCsrcs() == hasCsrcs);                                              \
		REQUIRE(packet->HasHeaderExtension() == hasHeaderExtension); \
		REQUIRE( \
		  helpers::AreBuffersEqual(packet->GetBuffer(), packet->GetLength(), buffer, length) == true); \
		REQUIRE_THROWS_AS( \
		  const_cast<Packet*>(packet)->Serialize(ThrowBuffer, length - 1), MediaSoupError); \
		REQUIRE_THROWS_AS(packet->Clone(ThrowBuffer, length - 1), MediaSoupError); \ 	} while (false)

#endif
