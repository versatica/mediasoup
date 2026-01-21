#include "api/network_state_predictor.h"
#define MS_CLASS "RTC::ICE::StunPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include "RTC/ICE/StunPacket.hpp"
#include <cstdio> // std::snprintf()

namespace RTC
{
	namespace ICE
	{
		/* Class variables. */

		const uint8_t StunPacket::MagicCookie[] = { 0x21, 0x12, 0xA4, 0x42 };

		/* Class methods. */

		bool StunPacket::IsStun(const uint8_t* buffer, size_t bufferLength)
		{
			// clang-format off
			return (
				// STUN headers are 20 bytes.
				(bufferLength >= StunPacket::FixedHeaderLength) &&
				// @see RFC 7983.
				(buffer[0] < 3) &&
				// Magic cookie must match.
				(buffer[4] == StunPacket::MagicCookie[0]) && (buffer[5] == StunPacket::MagicCookie[1]) &&
				(buffer[6] == StunPacket::MagicCookie[2]) && (buffer[7] == StunPacket::MagicCookie[3])
			);
			// clang-format on
		}

		StunPacket* StunPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (!StunPacket::IsStun(buffer, bufferLength))
			{
				MS_WARN_TAG(ice, "not a STUN Packet");

				return nullptr;
			}

			// Get STUN Message Type field.
			const uint16_t msgType = Utils::Byte::Get2Bytes(buffer, 0);

			// Get Message Length field.
			const uint16_t msgLength = Utils::Byte::Get2Bytes(buffer, 2);

			// Message Length field must be total length minus header's 20 bytes, and
			// must be multiple of 4 Bytes.
			if (
			  static_cast<size_t>(msgLength) != bufferLength - StunPacket::FixedHeaderLength ||
			  !Utils::Byte::IsPaddedTo4Bytes(msgLength))
			{
				MS_WARN_TAG(
				  ice,
				  "Message Length field + %zu does not match given buffer length or it's not multiple of 4 bytes, packet discarded",
				  StunPacket::FixedHeaderLength);

				return nullptr;
			}

			// Get STUN class.
			const auto msgClass =
			  static_cast<StunPacket::Class>(((buffer[0] & 0x01) << 1) | ((buffer[1] & 0x10) >> 4));

			// Get STUN method.
			const auto msgMethod = static_cast<StunPacket::Method>(
			  (msgType & 0x000f) | ((msgType & 0x00e0) >> 1) | ((msgType & 0x3E00) >> 2));

			auto* packet = new StunPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// `bufferLength` must be the exact length of the Packet, so let's assign
			// it immediately.
			packet->SetLength(bufferLength);

			packet->SetClass(msgClass);
			packet->SetMethod(msgMethod);

			// TODO
		}

		/* Instance methods. */

		StunPacket::StunPacket(uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();

			// TODO
		}

		StunPacket::~StunPacket()
		{
			MS_TRACE();
		}

		void StunPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<ICE::StunPacket>");

			// TODO

			MS_DUMP_CLEAN(indentation, "<ICE::StunPacket>");
		}

		StunPacket* StunPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedPacket = new StunPacket(buffer, bufferLength);

			Serializable::CloneInto(clonedPacket);

			// TODO: Copy pointers and so on.

			return clonedPacket;
		}

		void StunPacket::SetClass(StunPacket::Class klass)
		{
			MS_TRACE();

			this->klass = klass;
		}

		void StunPacket::SetMethod(StunPacket::Method method)
		{
			MS_TRACE();

			this->method = method;
		}
	} // namespace ICE
} // namespace RTC
