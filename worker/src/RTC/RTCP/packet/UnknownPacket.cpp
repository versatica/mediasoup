#define MS_CLASS "RTC::RTCP::UnknownPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RTCP/packet/UnknownPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RTCP
	{
		/* Class methods. */

		UnknownPacket* UnknownPacket::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Packet::PacketType packetType;
			size_t packetLength;
			uint8_t padding;

			if (!Packet::IsPacket(buffer, bufferLength, packetType, packetLength, padding))
			{
				return nullptr;
			}

			return UnknownPacket::ParseStrict(buffer, bufferLength, packetLength);
		}

		UnknownPacket* UnknownPacket::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, size_t packetLength)
		{
			MS_TRACE();

			auto* const packet = new UnknownPacket(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			packet->SetLength(packetLength);

			return packet;
		}

		/* Instance methods. */

		UnknownPacket::UnknownPacket(uint8_t* buffer, size_t bufferLength)
		  : Packet(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(Packet::CommonHeaderLength);
		}

		UnknownPacket::~UnknownPacket()
		{
			MS_TRACE();
		}

		void UnknownPacket::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<RTCP::UnknownPacket>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(
			  indentation,
			  "  unknown value length: %zu (has unknown value: %s)",
			  GetUnknownValueLength(),
			  HasUnknownValue() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</RTCP::UnknownPacket>");
		}

		UnknownPacket* UnknownPacket::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* const clonedPacket = new UnknownPacket(buffer, bufferLength);

			CloneInto(clonedPacket);

			return clonedPacket;
		}

		UnknownPacket* UnknownPacket::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* const softClonedPacket = new UnknownPacket(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedPacket);

			return softClonedPacket;
		}
	} // namespace RTCP
} // namespace RTC
