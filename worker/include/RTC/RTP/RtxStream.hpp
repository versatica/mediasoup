#ifndef MS_RTC_RTP_RTX_STREAM_HPP
#define MS_RTC_RTP_RTX_STREAM_HPP

#include "common.hpp"
#include "FBS/rtxStream.h"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "SharedInterface.hpp"
#include <string>

namespace RTC
{
	namespace RTP
	{
		class RtxStream
		{
		public:
			struct Params
			{
				flatbuffers::Offset<FBS::RtxStream::Params> FillBuffer(
				  flatbuffers::FlatBufferBuilder& builder) const;

				uint32_t ssrc{ 0 };
				uint8_t payloadType{ 0 };
				RTC::RtpCodecMimeType mimeType;
				uint32_t clockRate{ 0 };
				std::string rrid;
				std::string cname;
			};

		private:
			/**
			 * Data of a received RTCP Sender Report needed to report LSR and DLSR back in a
			 * Receiver Report.
			 */
			struct SenderReportTiming
			{
				/**
				 * Middle 32 bits out of 64 in the NTP timestamp of the Sender Report.
				 */
				uint32_t compactNtp;
				/**
				 * Local time at which the Sender Report arrived.
				 */
				uint64_t receivedMs;
			};

		public:
			explicit RtxStream(SharedInterface* shared, RTP::RtxStream::Params& params);

			virtual ~RtxStream();

		public:
			flatbuffers::Offset<FBS::RtxStream::RtxDump> FillBuffer(
			  flatbuffers::FlatBufferBuilder& builder) const;

			uint32_t GetSsrc() const
			{
				return this->params.ssrc;
			}

			uint8_t GetPayloadType() const
			{
				return this->params.payloadType;
			}

			const RTC::RtpCodecMimeType& GetMimeType() const
			{
				return this->params.mimeType;
			}

			uint32_t GetClockRate() const
			{
				return this->params.clockRate;
			}

			const std::string& GetRrid() const
			{
				return this->params.rrid;
			}

			const std::string& GetCname() const
			{
				return this->params.cname;
			}

			uint8_t GetFractionLost() const
			{
				return this->fractionLost;
			}

			float GetLossPercentage() const
			{
				return static_cast<float>(this->fractionLost) * 100 / 256;
			}

			size_t GetPacketsDiscarded() const
			{
				return this->packetsDiscarded;
			}

			bool ReceivePacket(const RTP::Packet* packet);

			RTC::RTCP::ReceiverReport* GetRtcpReceiverReport();

			void ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report);

		protected:
			bool UpdateSeq(const RTP::Packet* packet);

			uint32_t GetExpectedPackets() const
			{
				return (this->cycles + this->maxSeq) - this->baseSeq + 1;
			}

		private:
			void InitSeq(uint16_t seq);

		protected:
			// Given as argument.
			SharedInterface* shared{ nullptr };
			Params params;
			// Others.
			//   https://tools.ietf.org/html/rfc3550#appendix-A.1 stuff.
			uint16_t maxSeq{ 0u };      // Highest seq. number seen.
			uint32_t cycles{ 0u };      // Shifted count of seq. number cycles.
			uint32_t baseSeq{ 0u };     // Base seq number.
			uint32_t badSeq{ 0u };      // Last 'bad' seq number + 1.
			uint32_t maxPacketTs{ 0u }; // Highest timestamp seen.
			uint64_t maxPacketMs{ 0u }; // When the packet with highest timestammp was seen.
			int32_t packetsLost{ 0 };
			uint8_t fractionLost{ 0u };
			size_t packetsDiscarded{ 0u };
			size_t packetsCount{ 0u };

		private:
			// Whether at least a RTP packet has been received.
			bool started{ false };
			// Fields for generating Receiver Reports.
			uint32_t expectedPrior{ 0u };
			uint32_t receivedPrior{ 0u };
			// Timing data of the most recent Sender Report received.
			std::optional<SenderReportTiming> lastSenderReportTiming;
			int32_t reportedPacketsLost{ 0 };
		};
	} // namespace RTP
} // namespace RTC

#endif
