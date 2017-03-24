#ifndef MS_RTC_RTCP_FEEDBACK_REMB_HPP
#define MS_RTC_RTCP_FEEDBACK_REMB_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include <vector>

/* draft-alvestrand-rmcat-remb-03
 * RTCP message for Receiver Estimated Maximum Bitrate (REMB)

    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P| FMT=15  |   PT=206      |             length            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
0  |                  SSRC of packet sender                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
4  |                  SSRC of media source                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
8  |  Unique identifier 'R' 'E' 'M' 'B'                            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
12 |  Num SSRC     | BR Exp    |  BR Mantissa                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
16 |   SSRC feedback                                               |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  ...                                                          |
   */

namespace RTC { namespace RTCP
{
	class FeedbackPsRembPacket
		: public FeedbackPsAfbPacket
	{
	public:
		// 'R' 'E' 'M' 'B'.
		static uint32_t UniqueIdentifier;

	public:
		static FeedbackPsRembPacket* Parse(const uint8_t* data, size_t len);

	public:
		// Parsed Report. Points to an external data.
		explicit FeedbackPsRembPacket(CommonHeader* commonHeader);
		FeedbackPsRembPacket(uint32_t sender_ssrc, uint32_t media_ssrc);
		virtual ~FeedbackPsRembPacket() {};

		bool IsCorrect();
		void SetBitrate(uint64_t bitrate);
		void SetSsrcs(const std::vector<uint32_t>& ssrcs);
		uint64_t GetBitrate();
		const std::vector<uint32_t>& GetSsrcs();

	/* Pure virtual methods inherited from Packet. */
	public:
		virtual void Dump() const override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetSize() const override;

	private:
		std::vector<uint32_t> ssrcs;
		// Bitrate represented in bps
		uint64_t bitrate;
		bool isCorrect = true;
	};

	/* Inline instance methods. */

	inline
	FeedbackPsRembPacket::FeedbackPsRembPacket(uint32_t sender_ssrc, uint32_t media_ssrc):
		FeedbackPsAfbPacket(sender_ssrc, media_ssrc, FeedbackPsAfbPacket::Application::REMB)
	{}

	inline
	bool FeedbackPsRembPacket::IsCorrect()
	{
		return this->isCorrect;
	}

	inline
	void FeedbackPsRembPacket::SetBitrate(uint64_t bitrate)
	{
		this->bitrate = bitrate;
	}

	inline
	void FeedbackPsRembPacket::SetSsrcs(const std::vector<uint32_t>& ssrcs)
	{
		this->ssrcs = ssrcs;
	}

	inline
	uint64_t FeedbackPsRembPacket::GetBitrate()
	{
		return this->bitrate;
	}

	inline
	const std::vector<uint32_t>& FeedbackPsRembPacket::GetSsrcs()
	{
		return this->ssrcs;
	}

	inline
	size_t FeedbackPsRembPacket::GetSize() const
	{
		return FeedbackPsPacket::GetSize() + 8 + (sizeof(uint32_t) * this->ssrcs.size());
	}
}}

#endif
