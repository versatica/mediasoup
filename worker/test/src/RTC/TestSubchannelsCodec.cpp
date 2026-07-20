#include "common.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SubchannelsCodec.hpp"
#include "Utils.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

SCENARIO("SubchannelsCodec", "[subchannels]")
{
	// Build a message with the given payload.
	auto makeMessage = [](const std::vector<uint8_t>& payload)
	{
		return RTC::SCTP::Message(/*streamId*/ 1, /*ppid*/ 51, payload);
	};

	// Copy the current payload of a message into a std::vector.
	auto payloadToVector = [](const RTC::SCTP::Message& message)
	{
		const auto payload = message.GetPayload();

		return std::vector<uint8_t>(payload.begin(), payload.end());
	};

	SECTION(
	  "EncodeSubchannels() encodes subchannels and requiredSubchannel and DecodeSubchannels() decodes them back")
	{
		const std::vector<uint8_t> originalPayload = { 0x01, 0x02, 0x03, 0x04 };

		auto message = makeMessage(originalPayload);

		const std::vector<uint16_t> subchannels          = { 10, 20, 300 };
		const std::optional<uint16_t> requiredSubchannel = 5;

		REQUIRE(
		  RTC::SubchannelsCodec::EncodeSubchannels(message, subchannels, requiredSubchannel) == true);

		// 8 (Magic Token) + 2 (subchannelsCount) + 3 * 2 (subchannels) + 2 (requiredSubchannel).
		REQUIRE(message.GetPayloadLength() == originalPayload.size() + 8 + 2 + (3 * 2) + 2);

		std::vector<uint16_t> decodedSubchannels;
		std::optional<uint16_t> decodedRequiredSubchannel;

		REQUIRE(
		  RTC::SubchannelsCodec::DecodeSubchannels(
		    message, decodedSubchannels, decodedRequiredSubchannel) == true);

		REQUIRE(decodedSubchannels == subchannels);
		REQUIRE(decodedRequiredSubchannel == requiredSubchannel);
		// The header must have been removed, leaving the original payload.
		REQUIRE(payloadToVector(message) == originalPayload);
	}

	SECTION("EncodeSubchannels() encodes subchannels without requiredSubchannel")
	{
		const std::vector<uint8_t> originalPayload = { 0xAA, 0xBB };

		auto message = makeMessage(originalPayload);

		const std::vector<uint16_t> subchannels = { 1, 2 };

		REQUIRE(RTC::SubchannelsCodec::EncodeSubchannels(message, subchannels, std::nullopt) == true);

		// 8 (Magic Token) + 2 (subchannelsCount) + 2 * 2 (subchannels).
		REQUIRE(message.GetPayloadLength() == originalPayload.size() + 8 + 2 + (2 * 2));

		std::vector<uint16_t> decodedSubchannels;
		std::optional<uint16_t> decodedRequiredSubchannel;

		REQUIRE(
		  RTC::SubchannelsCodec::DecodeSubchannels(
		    message, decodedSubchannels, decodedRequiredSubchannel) == true);

		REQUIRE(decodedSubchannels == subchannels);
		REQUIRE(decodedRequiredSubchannel.has_value() == false);
		REQUIRE(payloadToVector(message) == originalPayload);
	}

	SECTION("EncodeSubchannels() encodes requiredSubchannel without subchannels")
	{
		const std::vector<uint8_t> originalPayload = { 0xAA, 0xBB, 0xCC };

		auto message = makeMessage(originalPayload);

		const std::optional<uint16_t> requiredSubchannel = 4321;

		REQUIRE(RTC::SubchannelsCodec::EncodeSubchannels(message, {}, requiredSubchannel) == true);

		// 8 (Magic Token) + 2 (subchannelsCount) + 2 (requiredSubchannel).
		REQUIRE(message.GetPayloadLength() == originalPayload.size() + 8 + 2 + 2);

		std::vector<uint16_t> decodedSubchannels;
		std::optional<uint16_t> decodedRequiredSubchannel;

		REQUIRE(
		  RTC::SubchannelsCodec::DecodeSubchannels(
		    message, decodedSubchannels, decodedRequiredSubchannel) == true);

		REQUIRE(decodedSubchannels.empty() == true);
		REQUIRE(decodedRequiredSubchannel == requiredSubchannel);
		REQUIRE(payloadToVector(message) == originalPayload);
	}

	SECTION("EncodeSubchannels() does nothing if there is nothing to encode")
	{
		const std::vector<uint8_t> originalPayload = { 0x01, 0x02 };

		auto message = makeMessage(originalPayload);

		REQUIRE(RTC::SubchannelsCodec::EncodeSubchannels(message, {}, std::nullopt) == false);

		// The message must remain untouched.
		REQUIRE(payloadToVector(message) == originalPayload);
	}

	SECTION("DecodeSubchannels() does nothing if the message does not start with the Magic Token")
	{
		const std::vector<uint8_t> originalPayload(12);

		auto message = makeMessage(originalPayload);

		std::vector<uint16_t> decodedSubchannels;
		std::optional<uint16_t> decodedRequiredSubchannel;

		REQUIRE(
		  RTC::SubchannelsCodec::DecodeSubchannels(
		    message, decodedSubchannels, decodedRequiredSubchannel) == false);

		REQUIRE(decodedSubchannels.empty() == true);
		REQUIRE(decodedRequiredSubchannel.has_value() == false);
		REQUIRE(payloadToVector(message) == originalPayload);
	}

	SECTION("DecodeSubchannels() does nothing if the message is shorter than the Magic Token")
	{
		const std::vector<uint8_t> originalPayload = { 0x01, 0x02 };

		auto message = makeMessage(originalPayload);

		std::vector<uint16_t> decodedSubchannels;
		std::optional<uint16_t> decodedRequiredSubchannel;

		REQUIRE(
		  RTC::SubchannelsCodec::DecodeSubchannels(
		    message, decodedSubchannels, decodedRequiredSubchannel) == false);

		REQUIRE(decodedSubchannels.empty() == true);
		REQUIRE(decodedRequiredSubchannel.has_value() == false);
		REQUIRE(payloadToVector(message) == originalPayload);
	}

	SECTION(
	  "DecodeSubchannels() ignores a truncated message that announces more subchannels than present")
	{
		// Craft a payload with the Magic Token (no requiredSubchannelFlag) that
		// announces 3 subchannels but does not carry their bytes.
		std::vector<uint8_t> originalPayload(10);

		Utils::Byte::Set8Bytes(originalPayload.data(), 0, RTC::SubchannelsCodec::MagicToken);
		Utils::Byte::Set2Bytes(originalPayload.data(), 8, /*subchannelsCount*/ 3);

		auto message = makeMessage(originalPayload);

		std::vector<uint16_t> decodedSubchannels;
		std::optional<uint16_t> decodedRequiredSubchannel;

		REQUIRE(
		  RTC::SubchannelsCodec::DecodeSubchannels(
		    message, decodedSubchannels, decodedRequiredSubchannel) == false);

		REQUIRE(decodedSubchannels.empty() == true);
		REQUIRE(decodedRequiredSubchannel.has_value() == false);
		REQUIRE(payloadToVector(message) == originalPayload);
	}

	SECTION(
	  "DecodeSubchannels() ignores a message whose requiredSubchannelFlag is set but has no room for the requiredSubchannel field")
	{
		// Craft a payload with the Magic Token and requiredSubchannelFlag set,
		// announcing 1 subchannel (present) but without the requiredSubchannel bytes.
		std::vector<uint8_t> originalPayload(12);

		Utils::Byte::Set8Bytes(
		  originalPayload.data(),
		  0,
		  RTC::SubchannelsCodec::MagicToken | RTC::SubchannelsCodec::RequiredSubchannelFlagMask);
		Utils::Byte::Set2Bytes(originalPayload.data(), 8, /*subchannelsCount*/ 1);
		Utils::Byte::Set2Bytes(originalPayload.data(), 10, /*subchannel*/ 42);

		auto message = makeMessage(originalPayload);

		std::vector<uint16_t> decodedSubchannels;
		std::optional<uint16_t> decodedRequiredSubchannel;

		REQUIRE(
		  RTC::SubchannelsCodec::DecodeSubchannels(
		    message, decodedSubchannels, decodedRequiredSubchannel) == false);

		REQUIRE(decodedSubchannels.empty() == true);
		REQUIRE(decodedRequiredSubchannel.has_value() == false);
		REQUIRE(payloadToVector(message) == originalPayload);
	}

	SECTION("EncodeSubchannels() and DecodeSubchannels() work with an empty original payload")
	{
		auto message = makeMessage({});

		const std::vector<uint16_t> subchannels          = { 7 };
		const std::optional<uint16_t> requiredSubchannel = 7;

		REQUIRE(
		  RTC::SubchannelsCodec::EncodeSubchannels(message, subchannels, requiredSubchannel) == true);

		std::vector<uint16_t> decodedSubchannels;
		std::optional<uint16_t> decodedRequiredSubchannel;

		REQUIRE(
		  RTC::SubchannelsCodec::DecodeSubchannels(
		    message, decodedSubchannels, decodedRequiredSubchannel) == true);

		REQUIRE(decodedSubchannels == subchannels);
		REQUIRE(decodedRequiredSubchannel == requiredSubchannel);
		REQUIRE(message.GetPayloadLength() == 0);
	}
}
