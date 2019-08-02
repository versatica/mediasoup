#ifndef MS_RTC_RTCP_FEEDBACK_RTP_TRANSPORT_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_TRANSPORT_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
#include "RTC/SeqManager.hpp"
#include <vector>

/* RTP Extensions for Transport-wide Congestion Control
 * draft-holmer-rmcat-transport-wide-cc-extensions-01

   0               1               2               3
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |V=2|P|  FMT=15 |    PT=205     |           length              |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                     SSRC of packet sender                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                      SSRC of media source                     |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |      base sequence number     |      packet status count      |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |                 reference time                | fb pkt. count |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          packet chunk         |         packet chunk          |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  .                                                               .
  .                                                               .
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |         packet chunk          |  recv delta   |  recv delta   |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  .                                                               .
  .                                                               .
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |           recv delta          |  recv delta   | zero padding  |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

 */

namespace RTC
{
	namespace RTCP
	{
		// RTP Transport packet declaration.
		class FeedbackRtpTransportPacket : public FeedbackRtpPacket
		{
		public:
			static size_t fixedHeaderSize;
			static uint16_t maxMissingPackets;
			static uint16_t maxPacketStatusCount;
			static uint16_t maxPacketDelta;

		public:
			static FeedbackRtpTransportPacket* Parse(const uint8_t* data, size_t len);

		public:
			FeedbackRtpTransportPacket(uint32_t senderSsrc, uint32_t mediaSsrc);
			FeedbackRtpTransportPacket(CommonHeader* commonHeader);
			~FeedbackRtpTransportPacket() override = default;

		public:
			bool AddPacket(uint16_t wideSeqNumber, uint64_t timestamp, size_t maxRtcpPacketLen);
			bool IsFull();
			bool IsSerializationCompleted();

			uint16_t GetBaseSequenceNumber() const;
			uint16_t GetPacketStatusCount() const;
			uint32_t GetReferenceTime() const;
			uint8_t GetFeedbackPacketCount() const;
			void SetFeedbackPacketCount(uint8_t count);

			/* Pure virtual methods inherited from Packet. */
		public:
			void Dump() const override;
			size_t Serialize(uint8_t* buffer) override;
			size_t GetSize() const override;

		private:
			enum Status : uint8_t
			{
				NotReceived = 0,
				SmallDelta,
				LargeDelta,
				Reserved,
				None
			};

			struct ReceivedPacket
			{
				ReceivedPacket(uint16_t sequenceNumber, uint16_t delta)
				  : sequenceNumber(sequenceNumber),
					  delta(delta)
				{
				}

				uint16_t sequenceNumber;
				uint16_t delta;
			};

			struct Context
			{
				bool allSameStatus              = true;
				Status currentStatus            = Status::None;
				std::vector<Status> statuses;
			};

			class Chunk
			{
			public:
				Chunk()          = default;
				virtual ~Chunk() = default;

			public:
				virtual size_t Serialize(uint8_t* buffer) = 0;
			};

			class RunLengthChunk : public Chunk
			{
			public:
				RunLengthChunk(Status status, uint16_t count) : status(status), count(count)
				{
				}
				RunLengthChunk(uint16_t buffer);

			public:
				size_t Serialize(uint8_t* buffer) override;

			private:
				Status status;
				uint16_t count;
			};

			class TwoBitVectorChunk : public Chunk
			{
			public:
				TwoBitVectorChunk(std::vector<Status> statuses)
					: statuses(statuses)
				{
				}
				TwoBitVectorChunk(uint16_t buffer);

			public:
				size_t Serialize(uint8_t* buffer) override;

			private:
				std::vector<Status> statuses;
			};

		private:
			bool FillChunk(uint16_t sequenceNumber, uint16_t delta);
			RunLengthChunk* CreateRunLengthChunk(Status status, uint16_t missingPackets);
			TwoBitVectorChunk* CreateTwoBitVectorChunk(std::vector<Status>& statuses);
			bool CheckMissingPackets(uint16_t anteriorSequenceNumber, uint16_t posteriorSecuenceNumber);
			bool CheckDelta(uint16_t anteriorTimestamp, uint16_t posteriorTimestamp);
			bool CheckSize(size_t maxRtcpPacketLen);

		private:
			uint16_t baseSequenceNumber{ 0 };
			uint16_t packetStatusCount{ 0 };
			uint32_t referenceTimeMs{ 0 };
			uint8_t feedbackPacketCount{ 0 };
			std::vector<ReceivedPacket> receivedPackets;
			std::vector<Chunk*> chunks;
			std::vector<uint16_t> deltas;
			uint16_t lastSequenceNumber{ 0 };
			uint64_t lastTimestamp{ 0 };
			Context context;
			size_t size{ 0 };
		};

		/* Inline instance methods. */
		inline FeedbackRtpTransportPacket::FeedbackRtpTransportPacket(uint32_t senderSsrc, uint32_t mediaSsrc)
		  : FeedbackRtpPacket(RTCP::FeedbackRtp::MessageType::TCC, senderSsrc, mediaSsrc)
		{
		}

		inline bool FeedbackRtpTransportPacket::IsFull()
		{
			return this->packetStatusCount == maxPacketStatusCount;
		}

		inline uint16_t FeedbackRtpTransportPacket::GetBaseSequenceNumber() const
		{
			return this->baseSequenceNumber;
		}

		inline uint16_t FeedbackRtpTransportPacket::GetPacketStatusCount() const
		{
			return this->packetStatusCount;
		}

		inline uint32_t FeedbackRtpTransportPacket::GetReferenceTime() const
		{
			return this->referenceTimeMs;
		}

		inline uint8_t FeedbackRtpTransportPacket::GetFeedbackPacketCount() const
		{
			return this->feedbackPacketCount;
		}

		inline void FeedbackRtpTransportPacket::SetFeedbackPacketCount(uint8_t count)
		{
			this->feedbackPacketCount = count;
		}

		inline FeedbackRtpTransportPacket::RunLengthChunk* FeedbackRtpTransportPacket::CreateRunLengthChunk(
		  Status status, uint16_t missingPackets)
		{
			return new RunLengthChunk(status, missingPackets);
		}

		inline FeedbackRtpTransportPacket::TwoBitVectorChunk* FeedbackRtpTransportPacket::CreateTwoBitVectorChunk(
		  std::vector<Status>& statuses)
		{
			return new TwoBitVectorChunk(statuses);
		}

		inline size_t FeedbackRtpTransportPacket::GetSize() const
		{
			// Fixed packet size.
			size_t size = FeedbackRtpPacket::GetSize();

			size += fixedHeaderSize;

			// Size chunks and deltas represented in this packet.
			size += this->size;

			// 32 bits padding.
			size += (-size) & 3;

			return size;
		}
	} // namespace RTCP
} // namespace RTC

#endif
