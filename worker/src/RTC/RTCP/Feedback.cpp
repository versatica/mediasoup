#define MS_CLASS "RTC::RTCP::Feedback"

#include "RTC/RTCP/Feedback.h"
#include "Logger.h"
#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
namespace RTCP
{

	/* FeedbackPacket Instance methods. */

	FeedbackPacket::FeedbackPacket(CommonHeader* commonHeader):
		Packet(Type(commonHeader->packet_type)),
		size((size_t)(ntohs(commonHeader->length) + 1) * 4)
	{
		this->header = (Header*)(commonHeader + 1 /* 1 => sizeof(CommonHeader) */);
	}

	/* FeedbackPacket Instance methods. */

	size_t FeedbackPacket::Serialize(uint8_t* data)
	{
		MS_TRACE_STD();

		size_t offset = Packet::Serialize(data);

		// Copy the header.
		std::memcpy(data+offset, this->header, sizeof(Header));

		offset += sizeof(Header);

		// No children serialization yet.
		std::memcpy(data+offset, this->header+offset, this->size-offset);

		return size;
	}

	void FeedbackPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("\tsize: %zu", this->size);
		MS_WARN("\tsender_ssrc: %u", ntohl(this->header->s_ssrc));
		MS_WARN("\tmedia_ssrc: %u", ntohl(this->header->m_ssrc));
	}

	/* FeedbackPsPacket Class variables */
	std::map<FeedbackPsPacket::MessageType, std::string> FeedbackPsPacket::type2String =
		{
			{  PLI,   "PLI"   },
			{  SLI,   "SLI"   },
			{  RPSI,  "RPSI"  },
			{  FIR,   "FIR"   },
			{  TSTR,  "TSTR"  },
			{  TSTN,  "TSTN"  },
			{  VBCM,  "VBCM"  },
			{  PSLEI, "PSLEI" },
			{  ROI,   "ROI"   },
			{  AFB,   "AFB"   },
			{  EXT,   "EXT"   }
		};

	/* FeedbackPsPacket Class methods. */

	FeedbackPsPacket* FeedbackPsPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackPsPacket> packet(new FeedbackPsPacket(commonHeader));

		return packet.release();
	}

	const std::string& FeedbackPsPacket::Type2String(MessageType type)
	{
		static const std::string unknown("UNKNOWN");

		if (type2String.find(type) == type2String.end())
			return unknown;

		return type2String[type];
	}

	/* FeedbackPsPacket Instance methods. */

	FeedbackPsPacket::FeedbackPsPacket(CommonHeader* commonHeader):
		FeedbackPacket(commonHeader),
		messageType((MessageType)commonHeader->count)
	{
	}

	void FeedbackPsPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("<FeedbackPsPacket>");
		FeedbackPacket::Dump();
		MS_WARN("\tmessageType: %s", Type2String(this->messageType).c_str());
		MS_WARN("</FeedbackPsPacket>");
	}

	/* FeedbackRtpPacket Class variables */
	std::map<FeedbackRtpPacket::MessageType, std::string> FeedbackRtpPacket::type2String =
		{
			{ NACK,   "NACK"   },
			{ TMMBR,  "TMMBR"  },
			{ TMMBN,  "TMMBN"  },
			{ SR_REQ, "SR_REQ" },
			{ RAMS,   "RAMS"   },
			{ TLLEI,  "TLLEI"  },
			{ ECN_FB, "ECN_FB" },
			{ PS,     "PS"     },
			{ EXT,    "EXT"    }
		};

	/* FeedbackRtpPacket Class methods. */

	FeedbackRtpPacket* FeedbackRtpPacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE_STD();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN("not enough space for Feedback packet, discarded");
				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		std::auto_ptr<FeedbackRtpPacket> packet(new FeedbackRtpPacket(commonHeader));

		return packet.release();
	}

	const std::string& FeedbackRtpPacket::Type2String(MessageType type)
	{
		static const std::string unknown("UNKNOWN");

		if (type2String.find(type) == type2String.end())
			return unknown;

		return type2String[type];
	}

	/* FeedbackRtpPacket Instance methods. */

	FeedbackRtpPacket::FeedbackRtpPacket(CommonHeader* commonHeader):
		FeedbackPacket(commonHeader),
		messageType((MessageType)commonHeader->count)
	{
	}

	void FeedbackRtpPacket::Dump()
	{
		MS_TRACE_STD();

		if (!Logger::HasDebugLevel())
			return;

		FeedbackPacket::Dump();

		MS_WARN("<FeedbackRtpPacket>");
		FeedbackPacket::Dump();
		MS_WARN("\tmessageType: %s", Type2String(this->messageType).c_str());
		MS_WARN("</FeedbackRtpPacket>");
	}
}
}
