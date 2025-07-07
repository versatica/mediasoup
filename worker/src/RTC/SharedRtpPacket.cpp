#define MS_CLASS "RTC::SharedRtpPacket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SharedRtpPacket.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	SharedRtpPacket::SharedRtpPacket()
	  : sharedPtr(std::make_shared<std::unique_ptr<RTC::RtpPacket>>(nullptr))
	{
		MS_TRACE();
	}

	SharedRtpPacket::SharedRtpPacket(const RTC::RtpPacket* packet)
	  : sharedPtr(std::make_shared<std::unique_ptr<RTC::RtpPacket>>(nullptr))
	{
		MS_TRACE();

		if (packet)
		{
			this->sharedPtr->reset(packet->Clone());
		}
	}

	SharedRtpPacket::~SharedRtpPacket()
	{
		MS_TRACE();
	}

	void SharedRtpPacket::Dump(int indentation) const
	{
		MS_TRACE();

		MS_DUMP_CLEAN(indentation, "<SharedRtpPacket>");
		MS_DUMP_CLEAN(indentation, "  has packet: %s", HasPacket() ? "yes" : "no");
		if (HasPacket())
		{
			const auto* packet = GetPacket();

			MS_DUMP_CLEAN(indentation, "  ssrc: %" PRIu32, packet->GetSsrc());
			MS_DUMP_CLEAN(indentation, "  sequence number: %" PRIu16, packet->GetSequenceNumber());
			MS_DUMP_CLEAN(indentation, "  timestamp: %" PRIu32, packet->GetTimestamp());
			MS_DUMP_CLEAN(indentation, "  payload type: %" PRIu8, packet->GetPayloadType());
		}
		MS_DUMP_CLEAN(indentation, "</SharedRtpPacket>");
	}

	void SharedRtpPacket::Assign(const RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (packet)
		{
			this->sharedPtr->reset(packet->Clone());
		}
		else
		{
			this->sharedPtr->reset(nullptr);
		}
	}

	void SharedRtpPacket::Reset()
	{
		MS_TRACE();

		this->sharedPtr->reset(nullptr);
	}

	void SharedRtpPacket::AssertSamePacket(const RTC::RtpPacket* otherPacket) const
	{
		MS_TRACE();

		const auto* packet = GetPacket();

		if (!packet && !otherPacket)
		{
			return;
		}
		else if (packet && !otherPacket)
		{
			MS_ABORT("there is a packet in sharedPacket but given otherPacket doesn't have value");
		}
		else if (!packet && otherPacket)
		{
			MS_ABORT("there is no packet in sharedPacket but given otherPacket has value");
		}
		else
		{
			MS_ASSERT(
			  packet->GetSsrc() == otherPacket->GetSsrc(),
			  "SSRC %" PRIu32 " in packet in sharedPacket != SSRC %" PRIu32 " in otherPacket",
			  packet->GetSsrc(),
			  otherPacket->GetSsrc());

			MS_ASSERT(
			  packet->GetSequenceNumber() == otherPacket->GetSequenceNumber(),
			  "seq %" PRIu16 " in packet in sharedPacket != seq %" PRIu16 " in otherPacket",
			  packet->GetSequenceNumber(),
			  otherPacket->GetSequenceNumber());

			MS_ASSERT(
			  packet->GetTimestamp() == otherPacket->GetTimestamp(),
			  "timestamp %" PRIu16 " in packet in sharedPacket != timestamp %" PRIu16 " in otherPacket",
			  packet->GetTimestamp(),
			  otherPacket->GetTimestamp());
		}
	}
} // namespace RTC
