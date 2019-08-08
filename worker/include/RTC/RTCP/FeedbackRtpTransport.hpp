#ifndef MS_RTC_RTCP_FEEDBACK_RTP_TRANSPORT_HPP
#define MS_RTC_RTCP_FEEDBACK_RTP_TRANSPORT_HPP

#include "common.hpp"
#include "RTC/RTCP/FeedbackRtp.hpp"
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
			~FeedbackRtpTransportPacket();

		public:
			bool AddPacket(uint16_t wideSeqNumber, uint64_t timestamp, size_t maxRtcpPacketLen);
			bool IsFull();
			bool IsSerializable();
			uint16_t GetBaseSequenceNumber() const;
			uint16_t GetPacketStatusCount() const;
			uint32_t GetReferenceTime() const;
			uint8_t GetFeedbackPacketCount() const;
			uint16_t GetLastSequenceNumber() const;
			uint64_t GetLastTimestamp() const;
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
				  : sequenceNumber(sequenceNumber), delta(delta)
				{
				}

				uint16_t sequenceNumber;
				uint16_t delta;
			};

			struct Context
			{
				bool allSameStatus{ true };
				Status currentStatus{ Status::None };
				std::vector<Status> statuses;
			};

			struct Chunk
			{
				Chunk()          = default;
				virtual ~Chunk() = default;

				virtual void Dump() = 0;
				virtual size_t Serialize(uint8_t* buffer) = 0;
			};

			struct RunLengthChunk : public Chunk
			{
				RunLengthChunk(Status status, uint16_t count);
				RunLengthChunk(uint16_t buffer);

				void Dump() override;
				size_t Serialize(uint8_t* buffer) override;

				Status status{ Status::None };
				uint16_t count{ 0u };
			};

			struct TwoBitVectorChunk : public Chunk
			{
				TwoBitVectorChunk(std::vector<Status> statuses);
				TwoBitVectorChunk(uint16_t buffer);

				void Dump() override;
				size_t Serialize(uint8_t* buffer) override;

				std::vector<Status> statuses;
			};

		private:
			static std::map<Status, std::string> Status2String;

		private:
			void AddPendingChunks();
			void FillChunk(uint16_t previousSequenceNumber, uint16_t sequenceNumber, uint16_t delta);
			void CreateRunLengthChunk(Status status, uint16_t count);
			void CreateTwoBitVectorChunk(std::vector<Status>& statuses);
			bool CheckMissingPackets(uint16_t previousSequenceNumber, uint16_t nextSecuenceNumber);
			bool CheckDelta(uint16_t previousTimestamp, uint16_t nextTimestamp);
			bool CheckSize(size_t maxRtcpPacketLen);

		private:
			bool hasPreBase{ false };
			uint16_t preBaseSequenceNumber{ 0u };
			uint16_t baseSequenceNumber{ 0u };
			uint64_t referenceTimeMs{ 0u };
			uint16_t packetStatusCount{ 0u };
			uint8_t feedbackPacketCount{ 0u };
			std::vector<ReceivedPacket> receivedPackets;
			std::vector<Chunk*> chunks;
			std::vector<uint16_t> deltas;
			uint64_t lastTimestamp{ 0u };
			Context context;
			size_t size{ 0u };
		};

		/* Inline instance methods. */

		inline FeedbackRtpTransportPacket::FeedbackRtpTransportPacket(uint32_t senderSsrc, uint32_t mediaSsrc)
		  : FeedbackRtpPacket(RTC::RTCP::FeedbackRtp::MessageType::TCC, senderSsrc, mediaSsrc)
		{
		}

		inline bool FeedbackRtpTransportPacket::IsFull()
		{
			return this->packetStatusCount == FeedbackRtpTransportPacket::maxPacketStatusCount;
		}

		inline bool FeedbackRtpTransportPacket::IsSerializable()
		{
			return this->receivedPackets.size() > 0;
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

		inline uint16_t FeedbackRtpTransportPacket::GetLastSequenceNumber() const
		{
			if (this->receivedPackets.empty())
				return 0u;

			return this->receivedPackets.back().sequenceNumber;
		}

		inline uint64_t FeedbackRtpTransportPacket::GetLastTimestamp() const
		{
			return this->lastTimestamp;
		}

		inline void FeedbackRtpTransportPacket::SetFeedbackPacketCount(uint8_t count)
		{
			this->feedbackPacketCount = count;
		}

		inline size_t FeedbackRtpTransportPacket::GetSize() const
		{
			// Fixed packet size.
			size_t size = FeedbackRtpPacket::GetSize();

			size += FeedbackRtpTransportPacket::fixedHeaderSize;

			// Size chunks and deltas represented in this packet.
			size += this->size;

			// 32 bits padding.
			size += (-size) & 3;

			return size;
		}

		inline FeedbackRtpTransportPacket::RunLengthChunk::RunLengthChunk(Status status, uint16_t count)
			: status(status),
			  count(count)
		{
		}

		inline FeedbackRtpTransportPacket::TwoBitVectorChunk::TwoBitVectorChunk(std::vector<Status> statuses)
			: statuses(statuses)
		{
		}
	} // namespace RTCP
} // namespace RTC

#endif
