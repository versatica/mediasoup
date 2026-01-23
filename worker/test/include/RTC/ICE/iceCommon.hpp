#ifndef MS_TEST_RTC_ICE_COMMON_HPP
#define MS_TEST_RTC_ICE_COMMON_HPP

#include "common.hpp"
#include "MediaSoupErrors.hpp"          // IWYU pragma: export
#include "testHelpers.hpp"              // IWYU pragma: export in worker/test/include/
#include "RTC/ICE/StunPacket.hpp"       // IWYU pragma: export
#include <catch2/catch_test_macros.hpp> // IWYU pragma: export
#include <cstdlib>                      // std::malloc(), std::free()
#include <cstring>                      // std::memcpy()
#include <string>

using namespace RTC::ICE;

namespace RTC
{
	namespace ICE
	{
		// NOTE: We need to declare them here with `extern` and then define them in
		// common.cpp.
		extern thread_local uint8_t FactoryBuffer[66661];
		extern thread_local uint8_t SerializeBuffer[66662];
		extern thread_local uint8_t CloneBuffer[66663];
		extern thread_local uint8_t DataBuffer[66664];
		extern thread_local uint8_t ThrowBuffer[66665];

		void ResetBuffers();
	} // namespace ICE
} // namespace RTC

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define CHECK_STUN_PACKET(/*const StunPacket**/ packet,                                             \
                          /*const uint8_t**/ buffer,                                                \
                          /*size_t*/ bufferLength,                                                  \
                          /*size_t*/ length,                                                        \
                          /*StunPacket::Class*/ klass,                                              \
                          /*StunPacket::Method*/ method,                                            \
                          /*bool*/ hasUsername,                                                     \
                          /*std::string*/ username,                                                 \
                          /*bool*/ hasPriority,                                                     \
                          /*uint32_t*/ priority,                                                    \
                          /*bool*/ hasIceControlling,                                               \
                          /*uint64_t*/ iceControlling,                                              \
                          /*bool*/ hasIceControlled,                                                \
                          /*uint64_t*/ iceControlled,                                               \
                          /*bool*/ hasUseCandidate,                                                 \
                          /*bool*/ hasNomination,                                                   \
                          /*uint32_t*/ nomination,                                                  \
                          /*bool*/ hasSoftware,                                                     \
                          /*std::string*/ software,                                                 \
                          /*bool*/ hasErrorCode,                                                    \
                          /*uint16_t*/ errorCode,                                                   \
                          /*std::string*/ errorReason,                                              \
                          /*bool*/ hasMessageIntegrity,                                             \
                          /*bool*/ hasFingerprint)                                                  \
	do                                                                                                \
	{                                                                                                 \
		uint8_t* originalBuffer = static_cast<uint8_t*>(std::malloc(bufferLength));                     \
		std::memcpy(originalBuffer, buffer, bufferLength);                                              \
		REQUIRE(StunPacket::IsStun(buffer, bufferLength) == true);                                      \
		REQUIRE(packet);                                                                                \
		REQUIRE(packet->Validate(/*storeAttributes*/ false));                                           \
		REQUIRE(packet->GetBuffer() != nullptr);                                                        \
		REQUIRE(packet->GetBuffer() == buffer);                                                         \
		REQUIRE(packet->GetBufferLength() != 0);                                                        \
		REQUIRE(packet->GetBufferLength() == bufferLength);                                             \
		REQUIRE(packet->GetLength() != 0);                                                              \
		REQUIRE(packet->GetLength() == length);                                                         \
		REQUIRE(packet->GetClass() == klass);                                                           \
		REQUIRE(packet->GetMethod() == method);                                                         \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::USERNAME) == hasUsername);              \
		REQUIRE(packet->GetUsername() == username);                                                     \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::PRIORITY) == hasPriority);              \
		REQUIRE(packet->GetPriority() == priority);                                                     \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::ICE_CONTROLLING) == hasIceControlling); \
		REQUIRE(packet->GetIceControlling() == iceControlling);                                         \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::ICE_CONTROLLED) == hasIceControlled);   \
		REQUIRE(packet->GetIceControlled() == iceControlled);                                           \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::USE_CANDIDATE) == hasUseCandidate);     \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::NOMINATION) == hasNomination);          \
		REQUIRE(packet->GetNomination() == nomination);                                                 \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::SOFTWARE) == hasSoftware);              \
		REQUIRE(packet->GetSoftware() == software);                                                     \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::ERROR_CODE) == hasErrorCode);           \
		REQUIRE(packet->GetErrorCode() == errorCode);                                                   \
		REQUIRE(packet->GetErrorReason() == errorReason);                                               \
		REQUIRE(                                                                                        \
		  packet->HasAttribute(StunPacket::AttributeType::MESSAGE_INTEGRITY) == hasMessageIntegrity);   \
		REQUIRE(packet->HasAttribute(StunPacket::AttributeType::FINGERPRINT) == hasFingerprint);        \
		REQUIRE(helpers::AreBuffersEqual(buffer, bufferLength, originalBuffer, bufferLength) == true);  \
		REQUIRE_THROWS_AS(                                                                              \
		  const_cast<StunPacket*>(packet)->Serialize(ThrowBuffer, length - 1), MediaSoupError);         \
		REQUIRE_THROWS_AS(packet->Clone(ThrowBuffer, length - 1), MediaSoupError);                      \
		REQUIRE(packet->Validate(/*storeAttributes*/ false));                                           \
		std::free(originalBuffer);                                                                      \
	} while (false)

#endif
