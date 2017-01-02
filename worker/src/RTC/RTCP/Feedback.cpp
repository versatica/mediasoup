#define MS_CLASS "RTC::RTCP::Feedback"
// #define MS_LOG_DEV

#include "RTC/RTCP/Feedback.h"
// Feedback RTP.
#include "RTC/RTCP/FeedbackRtpNack.h"
#include "RTC/RTCP/FeedbackRtpTmmb.h"
#include "RTC/RTCP/FeedbackRtpSrReq.h"
#include "RTC/RTCP/FeedbackRtpTllei.h"
#include "RTC/RTCP/FeedbackRtpEcn.h"
// Feedback PS.
#include "RTC/RTCP/FeedbackPsPli.h"
#include "RTC/RTCP/FeedbackPsSli.h"
#include "RTC/RTCP/FeedbackPsRpsi.h"
#include "RTC/RTCP/FeedbackPsFir.h"
#include "RTC/RTCP/FeedbackPsTst.h"
#include "RTC/RTCP/FeedbackPsVbcm.h"
#include "RTC/RTCP/FeedbackPsLei.h"
#include "RTC/RTCP/FeedbackPsAfb.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	template <typename T>
	const std::string& FeedbackPacket<T>::MessageType2String(typename T::MessageType type)
	{
		static const std::string unknown("UNKNOWN");

		if (FeedbackPacket<T>::type2String.find(type) == FeedbackPacket<T>::type2String.end())
			return unknown;

		return FeedbackPacket<T>::type2String[type];
	}

	/* Instance methods. */

	template <typename T>
	FeedbackPacket<T>::FeedbackPacket(CommonHeader* commonHeader):
		Packet(RTCP::Type(commonHeader->packet_type)),
		messageType(typename T::MessageType(commonHeader->count))
	{
		// 1 => sizeof(CommonHeader).
		this->header = (Header*)(commonHeader + 1);
	}

	template <typename T>
	FeedbackPacket<T>::FeedbackPacket(typename T::MessageType messageType, uint32_t sender_ssrc, uint32_t media_ssrc):
		Packet(RtcpType),
		messageType(messageType)
	{
		this->raw = new uint8_t[sizeof(Header)];
		this->header = (Header*) this->raw;
		this->header->s_ssrc = htonl(sender_ssrc);
		this->header->m_ssrc = htonl(media_ssrc);
	}

	template <typename T>
	FeedbackPacket<T>::~FeedbackPacket<T>()
	{
		if (this->raw)
			delete this->raw;
	}

	/* Instance methods. */

	template <typename T>
	size_t FeedbackPacket<T>::Serialize(uint8_t* data)
	{
		MS_TRACE();

		size_t offset = Packet::Serialize(data);

		// Copy the header.
		std::memcpy(data+offset, this->header, sizeof(Header));

		return offset + sizeof(Header);
	}

	template <typename T>
	void FeedbackPacket<T>::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("  sender ssrc : %" PRIu32, ntohl(this->header->s_ssrc));
		MS_DEBUG_DEV("  media ssrc  : %" PRIu32, ntohl(this->header->m_ssrc));
		MS_DEBUG_DEV("  size        : %zu", this->GetSize());
	}

	/* Specialization for Ps class. */

	template<>
	Type FeedbackPacket<FeedbackPs>::RtcpType = RTCP::Type::PSFB;

	template<>
	std::map<FeedbackPs::MessageType, std::string> FeedbackPacket<FeedbackPs>::type2String =
	{
		{  FeedbackPs::PLI,   "PLI"   },
		{  FeedbackPs::SLI,   "SLI"   },
		{  FeedbackPs::RPSI,  "RPSI"  },
		{  FeedbackPs::FIR,   "FIR"   },
		{  FeedbackPs::TSTR,  "TSTR"  },
		{  FeedbackPs::TSTN,  "TSTN"  },
		{  FeedbackPs::VBCM,  "VBCM"  },
		{  FeedbackPs::PSLEI, "PSLEI" },
		{  FeedbackPs::ROI,   "ROI"   },
		{  FeedbackPs::AFB,   "AFB"   },
		{  FeedbackPs::EXT,   "EXT"   }
	};

	template<>
	FeedbackPacket<FeedbackPs>* FeedbackPacket<FeedbackPs>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		FeedbackPsPacket* packet = nullptr;

		switch (FeedbackPs::MessageType(commonHeader->count))
		{
			case FeedbackPs::PLI:
				packet = FeedbackPsPliPacket::Parse(data, len);
				break;

			case FeedbackPs::SLI:
				packet = FeedbackPsSliPacket::Parse(data, len);
				break;

			case FeedbackPs::RPSI:
				packet = FeedbackPsRpsiPacket::Parse(data, len);
				break;

			case FeedbackPs::FIR:
				packet = FeedbackPsFirPacket::Parse(data, len);
				break;

			case FeedbackPs::TSTR:
				packet = FeedbackPsTstrPacket::Parse(data, len);
				break;

			case FeedbackPs::TSTN:
				packet = FeedbackPsTstnPacket::Parse(data, len);
				break;

			case FeedbackPs::VBCM:
				packet = FeedbackPsVbcmPacket::Parse(data, len);
				break;

			case FeedbackPs::PSLEI:
				packet = FeedbackPsLeiPacket::Parse(data, len);
				break;

			case FeedbackPs::ROI:
				break;

			case FeedbackPs::AFB:
				packet = FeedbackPsAfbPacket::Parse(data, len);
				break;

			case FeedbackPs::EXT:
				break;

			default:
				MS_WARN_TAG(rtcp, "unknown RTCP PS Feedback message type [packet_type:%" PRIu8 "]", commonHeader->count);
		}

		return packet;
	}

	/* Specialization for Rtcp class. */

	template<>
	Type FeedbackPacket<FeedbackRtp>::RtcpType = RTCP::Type::RTPFB;

	template<>
	std::map<FeedbackRtp::MessageType, std::string> FeedbackPacket<FeedbackRtp>::type2String =
	{
		{ FeedbackRtp::NACK,   "NACK"   },
		{ FeedbackRtp::TMMBR,  "TMMBR"  },
		{ FeedbackRtp::TMMBN,  "TMMBN"  },
		{ FeedbackRtp::SR_REQ, "SR_REQ" },
		{ FeedbackRtp::RAMS,   "RAMS"   },
		{ FeedbackRtp::TLLEI,  "TLLEI"  },
		{ FeedbackRtp::ECN,    "ECN"    },
		{ FeedbackRtp::PS,     "PS"     },
		{ FeedbackRtp::EXT,    "EXT"    }
	};

	/* Class methods. */

	template<>
	FeedbackPacket<FeedbackRtp>* FeedbackPacket<FeedbackRtp>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (sizeof(CommonHeader) + sizeof(FeedbackPacket::Header) > len)
		{
				MS_WARN_TAG(rtcp, "not enough space for Feedback packet, discarded");

				return nullptr;
		}

		CommonHeader* commonHeader = (CommonHeader*)data;
		FeedbackRtpPacket* packet = nullptr;

		switch (FeedbackRtp::MessageType(commonHeader->count))
		{
			case FeedbackRtp::NACK:
				packet = FeedbackRtpNackPacket::Parse(data, len);
				break;

			case FeedbackRtp::TMMBR:
				packet = FeedbackRtpTmmbrPacket::Parse(data, len);
				break;

			case FeedbackRtp::TMMBN:
				packet = FeedbackRtpTmmbnPacket::Parse(data, len);
				break;

			case FeedbackRtp::SR_REQ:
				packet = FeedbackRtpSrReqPacket::Parse(data, len);
				break;

			case FeedbackRtp::RAMS:
				break;

			case FeedbackRtp::TLLEI:
				packet = FeedbackRtpTlleiPacket::Parse(data, len);
				break;

			case FeedbackRtp::ECN:
				packet = FeedbackRtpEcnPacket::Parse(data, len);
				break;

			case FeedbackRtp::PS:
				break;

			case FeedbackRtp::EXT:
				break;

			default:
				MS_WARN_TAG(rtcp, "unknown RTCP RTP Feedback message type [packet_type:%" PRIu8 "]", commonHeader->count);
		}

		return packet;
	}

	// Explicit instantiation to have all FeedbackPacket definitions in this file.
	template class FeedbackPacket<FeedbackPs>;
	template class FeedbackPacket<FeedbackRtp>;
}}
