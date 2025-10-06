#ifndef MS_RTC_RTC_LOGGER_HPP
#define MS_RTC_RTC_LOGGER_HPP

#include "common.hpp"
#include <absl/container/flat_hash_map.h>

namespace RTC
{
	namespace RtcLogger
	{
		class RtpPacket
		{
		public:
			enum class DiscardReason : uint8_t
			{
				NONE = 0,
				PRODUCER_NOT_FOUND,
				RECV_RTP_STREAM_NOT_FOUND,
				RECV_RTP_STREAM_DISCARDED,
				RECV_RTP_RTX_STREAM_DISCARDED,
				CONSUMER_INACTIVE,
				INVALID_TARGET_LAYER,
				UNSUPPORTED_PAYLOAD_TYPE,
				NOT_A_KEYFRAME,
				EMPTY_PAYLOAD,
				SPATIAL_LAYER_MISMATCH,
				PACKET_PREVIOUS_TO_SPATIAL_LAYER_SWITCH,
				DROPPED_BY_CODEC,
				TOO_HIGH_TIMESTAMP_EXTRA_NEEDED,
				SEND_RTP_STREAM_DISCARDED
			};

			static absl::flat_hash_map<DiscardReason, std::string> discardReason2String;

			RtpPacket()  = default;
			~RtpPacket() = default;
			void Sent();
			void Discarded(DiscardReason discardReason);

		private:
			void Log() const;
			void Clear();

		public:
			uint64_t timestamp{};
			std::string recvTransportId{};
			std::string sendTransportId{};
			std::string routerId{};
			std::string producerId{};
			std::string consumerId{};
			uint32_t recvRtpTimestamp{};
			uint32_t sendRtpTimestamp{};
			uint16_t recvSeqNumber{};
			uint16_t sendSeqNumber{};
			bool discarded{};
			DiscardReason discardReason{ DiscardReason::NONE };
		};
	}; // namespace RtcLogger
} // namespace RTC
#endif
