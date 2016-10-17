#ifndef MS_RTC_RTCP_FEEDBACK_H
#define MS_RTC_RTCP_FEEDBACK_H

#include "common.h"
#include "RTC/RTCP/Packet.h"

namespace RTC { namespace RTCP
{
	class FeedbackPacket
		: public Packet
	{
	public:
		/* Struct for RTP Feedback message. */
		struct Header
		{
			uint32_t s_ssrc;
			uint32_t m_ssrc;
		};

	public:
		// Parsed Report. Points to an external data.
		FeedbackPacket(CommonHeader* commonHeader);

		// Virtual methods inherited from Packet
		void Dump() override;
		virtual size_t GetCount() override = 0;
		virtual size_t Serialize(uint8_t* data) override;
		virtual size_t GetSize() override;

		uint32_t GetSenderSsrc();
		void SetSenderSsrc(uint32_t ssrc);
		uint32_t GetMediaSsrc();
		void SetMediaSsrc(uint32_t ssrc);

	private:
		Header* header = nullptr;
		size_t size;
	};


	class FeedbackPsPacket
		: public FeedbackPacket
	{

	public:
		typedef enum MessageType : uint8_t
		{
			PLI  = 1,
			SLI  = 2,
			RPSI = 3,
			FIR  = 4,
			TSTR = 5,
			TSTN = 6,
			VBCM = 7,
			PSLEI = 8,
			ROI  = 9,
			AFB  = 15,
			EXT  = 31
		} MessageType;

	public:
		static FeedbackPsPacket* Parse(const uint8_t* data, size_t len);
		static const std::string& Type2String(MessageType type);

	public:
		// Parsed Report. Points to an external data.
		FeedbackPsPacket(CommonHeader* commonHeader);

		void Dump() override;
		size_t GetCount() override;

	private:
		MessageType messageType;

	private:
		static std::map<FeedbackPsPacket::MessageType, std::string> type2String;
	};

	class FeedbackRtpPacket
		: public FeedbackPacket
	{

	public:
		typedef enum MessageType : uint8_t
		{
			NACK   = 1,
			TMMBR  = 3,
			TMMBN  = 4,
			SR_REQ = 5,
			RAMS   = 6,
			TLLEI  = 7,
			ECN_FB = 8,
			PS     = 9,
			EXT    = 31
		} MessageType;

	public:
		static FeedbackRtpPacket* Parse(const uint8_t* data, size_t len);
		static const std::string& Type2String(MessageType type);

	public:
		// Parsed Report. Points to an external data.
		FeedbackRtpPacket(CommonHeader* commonHeader);

		void Dump() override;
		size_t GetCount() override;

	private:
		MessageType messageType;

	private:
		static std::map<FeedbackRtpPacket::MessageType, std::string> type2String;
	};

	/* FeedbackPacket inline instance methods. */

	inline
	size_t FeedbackPacket::GetSize()
	{
		return this->size;
	}

	inline
	uint32_t FeedbackPacket::GetSenderSsrc()
	{
		return (uint32_t)ntohl(this->header->s_ssrc);
	}

	inline
	void FeedbackPacket::SetSenderSsrc(uint32_t ssrc)
	{
		this->header->s_ssrc = (uint32_t)htonl(ssrc);
	}

	inline
	uint32_t FeedbackPacket::GetMediaSsrc()
	{
		return (uint32_t)ntohl(this->header->m_ssrc);
	}

	inline
	void FeedbackPacket::SetMediaSsrc(uint32_t ssrc)
	{
		this->header->m_ssrc = (uint32_t)htonl(ssrc);
	}

	/* FeedbackPsPacket inline instance methods. */

	inline
	size_t FeedbackPsPacket::GetCount()
	{
		return (size_t)(uint8_t)this->messageType;
	}

	/* FeedbackRtpPacket inline instance methods. */

	inline
	size_t FeedbackRtpPacket::GetCount()
	{
		return (size_t)(uint8_t)this->messageType;
	}
}
}

#endif
