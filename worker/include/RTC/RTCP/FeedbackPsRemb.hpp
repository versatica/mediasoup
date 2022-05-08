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
  |                  SSRC of packet sender                        |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                  SSRC of media source                         |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Unique identifier 'R' 'E' 'M' 'B'                            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  Num SSRC     | BR Exp    |  BR Mantissa                      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |   SSRC feedback                                               |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |  ...                                                          |
 */

namespace RTC
{
	namespace RTCP
	{
		class FeedbackPsRembPacket : public FeedbackPsAfbPacket
		{
		public:
			// 'R' 'E' 'M' 'B'.
			static const uint32_t UniqueIdentifier{ 0x52454D42 };
			static const size_t UniqueIdentifierSize{ 4 };

		public:
			static FeedbackPsRembPacket* Parse(const uint8_t* data, size_t len);

		public:
			// Parsed Report. Points to an external data.
			FeedbackPsRembPacket(uint32_t senderSsrc, uint32_t mediaSsrc)
			  : FeedbackPsAfbPacket(senderSsrc, mediaSsrc, FeedbackPsAfbPacket::Application::REMB)
			{
			}
			FeedbackPsRembPacket(CommonHeader* commonHeader, size_t availableLen);
			~FeedbackPsRembPacket() override = default;

			bool IsCorrect() const
			{
				return this->isCorrect;
			}
			void SetBitrate(uint64_t bitrate)
			{
				this->bitrate = bitrate;
			}
			void SetSsrcs(const std::vector<uint32_t>& ssrcs)
			{
				this->ssrcs = ssrcs;
			}
			uint64_t GetBitrate()
			{
				return this->bitrate;
			}
			const std::vector<uint32_t>& GetSsrcs()
			{
				return this->ssrcs;
			}

			/* Pure virtual methods inherited from Packet. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override
			{
				return FeedbackPsPacket::GetSize() + 8 + (4u * this->ssrcs.size());
			}

		private:
			std::vector<uint32_t> ssrcs;
			// Bitrate represented in bps.
			uint64_t bitrate{ 0 };
			bool isCorrect{ true };
		};
	} // namespace RTCP
} // namespace RTC

#endif
