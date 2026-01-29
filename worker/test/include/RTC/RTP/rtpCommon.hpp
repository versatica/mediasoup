#ifndef MS_TEST_RTC_RTP_COMMON_HPP
#define MS_TEST_RTC_RTP_COMMON_HPP

#include "common.hpp"
#include "MediaSoupErrors.hpp"          // IWYU pragma: export
#include "testHelpers.hpp"              // IWYU pragma: export in worker/test/include/
#include "RTC/RTP/Packet.hpp"           // IWYU pragma: export
#include <catch2/catch_test_macros.hpp> // IWYU pragma: export
#include <cstdlib>                      // std::malloc(), std::free()
#include <cstring>                      // std::memcpy()

using namespace RTC::RTP;

namespace RTC
{
	namespace RTP
	{
		// NOTE: We need to declare them here with `extern` and then define them in
		// rtpCommon.cpp.
		// NOTE: Random size buffers because anyway we use sizeof(XxxxBuffer).
		extern thread_local uint8_t FactoryBuffer[66661];
		extern thread_local uint8_t SerializeBuffer[66662];
		extern thread_local uint8_t CloneBuffer[66663];
		extern thread_local uint8_t DataBuffer[66664];
		extern thread_local uint8_t ThrowBuffer[66665];

		void ResetBuffers();
	} // namespace RTP
} // namespace RTC

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_RTP_PACKET(/*const Packet**/ packet,                                                   \
                         /*const uint8_t**/ buffer,                                                  \
                         /*size_t*/ bufferLength,                                                    \
                         /*size_t*/ length,                                                          \
                         /*uint8_t*/ payloadType,                                                    \
                         /*bool*/ hasMarker,                                                         \
                         /*uint16_t*/ seqNumber,                                                     \
                         /*uint32_t*/ timestamp,                                                     \
                         /*uint32_t*/ ssrc,                                                          \
                         /*bool*/ hasCsrcs,                                                          \
                         /*bool*/ hasHeaderExtension,                                                \
                         /*size_t*/ headerExtensionValueLength,                                      \
                         /*bool*/ hasOneByteExtensions,                                              \
                         /*bool*/ hasTwoBytesExtensions,                                             \
                         /*bool*/ hasPayload,                                                        \
                         /*size_t*/ payloadLength,                                                   \
                         /*bool*/ hasPadding,                                                        \
                         /*uint8_t*/ paddingLength)                                                  \
	do                                                                                                 \
	{                                                                                                  \
		uint8_t* originalBuffer = static_cast<uint8_t*>(std::malloc(bufferLength));                      \
		std::memcpy(originalBuffer, buffer, bufferLength);                                               \
		REQUIRE(Packet::IsRtp(buffer, bufferLength) == true);                                            \
		REQUIRE(packet);                                                                                 \
		REQUIRE(packet->GetBuffer() != nullptr);                                                         \
		REQUIRE(packet->GetBuffer() == buffer);                                                          \
		REQUIRE(packet->GetBufferLength() != 0);                                                         \
		REQUIRE(packet->GetBufferLength() == bufferLength);                                              \
		REQUIRE(packet->GetLength() != 0);                                                               \
		REQUIRE(packet->GetLength() == length);                                                          \
		REQUIRE(static_cast<unsigned>(packet->GetVersion()) == 2);                                       \
		REQUIRE(static_cast<unsigned>(packet->GetPayloadType()) == payloadType);                         \
		REQUIRE(packet->HasMarker() == hasMarker);                                                       \
		REQUIRE(packet->GetSequenceNumber() == seqNumber);                                               \
		REQUIRE(packet->GetTimestamp() == timestamp);                                                    \
		REQUIRE(packet->GetSsrc() == ssrc);                                                              \
		REQUIRE(packet->HasCsrcs() == hasCsrcs);                                                         \
		REQUIRE(packet->HasHeaderExtension() == hasHeaderExtension);                                     \
		REQUIRE(packet->GetHeaderExtensionValueLength() == headerExtensionValueLength);                  \
		REQUIRE(packet->HasExtensions() == (hasOneByteExtensions || hasTwoBytesExtensions));             \
		REQUIRE(packet->HasOneByteExtensions() == hasOneByteExtensions);                                 \
		REQUIRE(packet->HasTwoBytesExtensions() == hasTwoBytesExtensions);                               \
		if (!packet->HasHeaderExtension())                                                               \
		{                                                                                                \
			REQUIRE(packet->GetHeaderExtensionValueLength() == 0);                                         \
			REQUIRE(packet->HasExtensions() == false);                                                     \
			REQUIRE(packet->HasOneByteExtensions() == false);                                              \
			REQUIRE(packet->HasTwoBytesExtensions() == false);                                             \
		}                                                                                                \
		REQUIRE(packet->HasPayload() == hasPayload);                                                     \
		REQUIRE(packet->GetPayloadLength() == payloadLength);                                            \
		size_t realPayloadLength;                                                                        \
		packet->GetPayload(realPayloadLength);                                                           \
		REQUIRE(realPayloadLength == payloadLength);                                                     \
		if (!packet->HasPayload())                                                                       \
		{                                                                                                \
			REQUIRE(packet->GetPayload() == nullptr);                                                      \
			REQUIRE(packet->GetPayloadLength() == 0);                                                      \
			size_t realPayloadLength;                                                                      \
			REQUIRE(packet->GetPayload(realPayloadLength) == nullptr);                                     \
			REQUIRE(realPayloadLength == 0);                                                               \
		}                                                                                                \
		REQUIRE(packet->HasPadding() == hasPadding);                                                     \
		REQUIRE(static_cast<unsigned>(packet->GetPaddingLength()) == paddingLength);                     \
		if (!packet->HasPadding())                                                                       \
		{                                                                                                \
			REQUIRE(static_cast<unsigned>(packet->GetPaddingLength()) == 0);                               \
		}                                                                                                \
		REQUIRE(packet->Validate(/*storeExtensions*/ false));                                            \
		REQUIRE_NOTHROW(packet->SetPayloadType(packet->GetPayloadType()));                               \
		REQUIRE_NOTHROW(packet->SetMarker(packet->HasMarker()));                                         \
		REQUIRE_NOTHROW(packet->SetSequenceNumber(packet->GetSequenceNumber()));                         \
		REQUIRE_NOTHROW(packet->SetTimestamp(packet->GetTimestamp()));                                   \
		REQUIRE_NOTHROW(packet->SetSsrc(packet->GetSsrc()));                                             \
		if (!packet->HasHeaderExtension())                                                               \
		{                                                                                                \
			REQUIRE_NOTHROW(packet->RemoveHeaderExtension());                                              \
		}                                                                                                \
		if (!hasPadding)                                                                                 \
		{                                                                                                \
			REQUIRE_NOTHROW(packet->SetPayload(packet->GetPayload(), packet->GetPayloadLength()));         \
			REQUIRE(helpers::AreBuffersEqual(buffer, bufferLength, originalBuffer, bufferLength) == true); \
			REQUIRE_NOTHROW(packet->SetPayloadLength(packet->GetPayloadLength()));                         \
			REQUIRE(helpers::AreBuffersEqual(buffer, bufferLength, originalBuffer, bufferLength) == true); \
		}                                                                                                \
		REQUIRE_THROWS_AS(packet->SetPayload(DataBuffer, packet->GetBufferLength()), MediaSoupError);    \
		REQUIRE_THROWS_AS(packet->SetPayloadLength(packet->GetBufferLength()), MediaSoupError);          \
		REQUIRE_NOTHROW(packet->SetPaddingLength(packet->GetPaddingLength()));                           \
		if (packet->IsPaddedTo4Bytes() && packet->GetPaddingLength() < 4)                                \
		{                                                                                                \
			REQUIRE_NOTHROW(packet->PadTo4Bytes());                                                        \
			REQUIRE(helpers::AreBuffersEqual(buffer, bufferLength, originalBuffer, bufferLength) == true); \
		}                                                                                                \
		REQUIRE(helpers::AreBuffersEqual(buffer, bufferLength, originalBuffer, bufferLength) == true);   \
		REQUIRE_THROWS_AS(                                                                               \
		  const_cast<Packet*>(packet)->Serialize(ThrowBuffer, length - 1), MediaSoupError);              \
		REQUIRE_THROWS_AS(packet->Clone(ThrowBuffer, length - 1), MediaSoupError);                       \
		REQUIRE(packet->Validate(/*storeExtensions*/ false));                                            \
		std::free(originalBuffer);                                                                       \
	} while (false)

#endif
