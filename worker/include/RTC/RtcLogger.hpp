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
			enum class DropReason : uint8_t
			{
				NONE = 0,
				PRODUCER_NOT_FOUND,
				RECV_RTP_STREAM_NOT_FOUND,
				RECV_RTP_STREAM_DISCARDED,
				CONSUMER_INACTIVE,
				INVALID_TARGET_LAYER,
				UNSUPPORTED_PAYLOAD_TYPE,
				NOT_A_KEYFRAME,
				SPATIAL_LAYER_MISMATCH,
				TOO_HIGH_TIMESTAMP_EXTRA_NEEDED,
				PACKET_PREVIOUS_TO_SPATIAL_LAYER_SWITCH,
				DROPPED_BY_CODEC,
				SEND_RTP_STREAM_DISCARDED,
			};

			static absl::flat_hash_map<DropReason, std::string> dropReason2String;

			RtpPacket()  = default;
			~RtpPacket() = default;
			void Sent();
			void Dropped(DropReason dropReason);

		private:
			void Log() const;
			void Clear();

		public:
			uint64_t timestamp;
			std::string recvTransportId{ "undefined" };
			std::string sendTransportId{ "undefined" };
			std::string routerId{ "undefined" };
			std::string producerId{ "undefined" };
			std::string consumerId{ "undefined" };
			uint32_t recvRtpTimestamp;
			uint32_t sendRtpTimestamp;
			uint16_t recvSeqNumber;
			uint16_t sendSeqNumber;
			bool dropped;
			DropReason dropReason{ DropReason::NONE };
		};
	}; // namespace RtcLogger
} // namespace RTC
#endif
