#define MS_CLASS "RTC::SubchannelsCodec"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SubchannelsCodec.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	/* Class methods. */

	bool SubchannelsCodec::EncodeSubchannels(
	  RTC::SCTP::Message& message,
	  const std::vector<uint16_t>& subchannels,
	  std::optional<uint16_t> requiredSubchannel)
	{
		MS_TRACE();

		// Nothing to encode.
		if (subchannels.empty() && !requiredSubchannel.has_value())
		{
			return false;
		}

		if (subchannels.size() > 0xFFFF)
		{
			MS_ERROR("too many subchannels to encode [count:%zu]", subchannels.size());

			return false;
		}

		const auto subchannelsCount       = static_cast<uint16_t>(subchannels.size());
		const bool requiredSubchannelFlag = requiredSubchannel.has_value();

		// Length of the header to prepend.
		size_t headerLen =
		  8 /* Magic Token */ + 2 /* subchannelsCount */ + (static_cast<size_t>(subchannelsCount) * 2);

		if (requiredSubchannelFlag)
		{
			headerLen += 2;
		}

		const auto payload = message.GetPayload();

		// Build the whole new payload at once (header followed by the original
		// payload) so it can be moved into the message with a single allocation.
		std::vector<uint8_t> newPayload(headerLen + payload.size());

		uint64_t magicToken = SubchannelsCodec::MagicToken;

		if (requiredSubchannelFlag)
		{
			magicToken |= SubchannelsCodec::RequiredSubchannelFlagMask;
		}

		size_t offset = 0;

		Utils::Byte::Set8Bytes(newPayload.data(), offset, magicToken);
		offset += 8;

		Utils::Byte::Set2Bytes(newPayload.data(), offset, subchannelsCount);
		offset += 2;

		for (const auto subchannel : subchannels)
		{
			Utils::Byte::Set2Bytes(newPayload.data(), offset, subchannel);
			offset += 2;
		}

		if (requiredSubchannelFlag)
		{
			Utils::Byte::Set2Bytes(newPayload.data(), offset, requiredSubchannel.value());
			offset += 2;
		}

		// Append the original payload right after the encoded header.
		std::ranges::copy(payload, newPayload.begin() + offset);

		message.SetPayload(std::move(newPayload));

		return true;
	}

	bool SubchannelsCodec::DecodeSubchannels(
	  RTC::SCTP::Message& message,
	  std::vector<uint16_t>& subchannels,
	  std::optional<uint16_t>& requiredSubchannel)
	{
		MS_TRACE();

		const auto payload = message.GetPayload();
		const size_t len   = payload.size();

		// Not even enough bytes to hold the Magic Token and `subchannelsCount` field,
		// so nothing to decode.
		if (len < 10)
		{
			return false;
		}

		const uint64_t magicToken = Utils::Byte::Get8Bytes(payload.data(), 0);

		// The message does not start with the Magic Token, so nothing to decode.
		if ((magicToken & ~SubchannelsCodec::RequiredSubchannelFlagMask) != SubchannelsCodec::MagicToken)
		{
			return false;
		}

		const bool requiredSubchannelFlag =
		  (magicToken & SubchannelsCodec::RequiredSubchannelFlagMask) != 0;

		const uint16_t subchannelsCount = Utils::Byte::Get2Bytes(payload.data(), 8);

		// Offset right after the Magic Token and subchannelsCount fields.
		size_t offset = 10;

		// Bytes needed to hold all announced subchannels and, if present, the
		// requiredSubchannel.
		size_t neededLen = offset + (static_cast<size_t>(subchannelsCount) * 2);

		if (requiredSubchannelFlag)
		{
			neededLen += 2;
		}

		if (len < neededLen)
		{
			MS_WARN_DEV("message too short to hold announced subchannels, ignoring");

			return false;
		}

		subchannels.reserve(subchannelsCount);

		for (uint16_t i = 0; i < subchannelsCount; ++i)
		{
			subchannels.push_back(Utils::Byte::Get2Bytes(payload.data(), offset));
			offset += 2;
		}

		if (requiredSubchannelFlag)
		{
			requiredSubchannel = Utils::Byte::Get2Bytes(payload.data(), offset);
			offset += 2;
		}

		// Remove the decoded header from the beginning of the message payload.
		message.RemovePayloadFront(offset);

		return true;
	}
} // namespace RTC
