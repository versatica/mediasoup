#define MS_CLASS "RTC::RtcLogger"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtcLogger.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace RtcLogger
	{
		// clang-format off
		absl::flat_hash_map<RtpPacket::DropReason, std::string> RtpPacket::dropReason2String = {
			{ RtpPacket::DropReason::NONE,                                    "None"                               },
			{ RtpPacket::DropReason::PRODUCER_NOT_FOUND,                      "ProducerNotFound"                   },
			{ RtpPacket::DropReason::RECV_RTP_STREAM_NOT_FOUND,               "RecvRtpStreamNotFound"              },
			{ RtpPacket::DropReason::RECV_RTP_STREAM_DISCARDED,               "RecvRtpStreamDiscarded"             },
			{ RtpPacket::DropReason::CONSUMER_INACTIVE,                       "ConsumerInactive"                   },
			{ RtpPacket::DropReason::INVALID_TARGET_LAYER,                    "InvalidTargetLayer"                 },
			{ RtpPacket::DropReason::UNSUPPORTED_PAYLOAD_TYPE,                "UnsupportedPayloadType"             },
			{ RtpPacket::DropReason::NOT_A_KEYFRAME,                          "NotAKeyframe"                       },
			{ RtpPacket::DropReason::SPATIAL_LAYER_MISMATCH,                  "SpatialLayerMismatch"               },
			{ RtpPacket::DropReason::TOO_HIGH_TIMESTAMP_EXTRA_NEEDED,         "TooHighTimestampExtraNeeded"        },
			{ RtpPacket::DropReason::PACKET_PREVIOUS_TO_SPATIAL_LAYER_SWITCH, "PacketPreviousToSpatialLayerSwitch" },
			{ RtpPacket::DropReason::DROPPED_BY_CODEC,                        "DroppedByCodec"                     },
			{ RtpPacket::DropReason::SEND_RTP_STREAM_DISCARDED,               "SendRtpStreamDiscarded"             },
		};
		// clang-format on

		void RtpPacket::Sent()
		{
			Log();
			Clear();
		}

		void RtpPacket::Dropped(DropReason dropReason)
		{
			this->dropped    = true;
			this->dropReason = dropReason;

			Log();
			Clear();
		}

		void RtpPacket::Log() const
		{
#ifdef MS_RTC_LOGGER_RTP
			std::cout << "{"
			          << "timestamp: " << this->timestamp << ", recvTransportId: '"
			          << this->recvTransportId << "'"
			          << ", sendTransportId: '" << this->sendTransportId << "'"
			          << ", routerId: '" << this->routerId << "'"
			          << ", producerId: '" << this->producerId << "'"
			          << ", consumerId: '" << this->consumerId << "'"
			          << ", rtpTimestamp: " << this->rtpTimestamp
			          << ", recvSeqNumber: " << this->recvSeqNumber
			          << ", sendSeqNumber: " << this->sendSeqNumber
			          << ", dropped: " << (this->dropped ? "true" : "false") << ", dropReason: '"
			          << dropReason2String[this->dropReason] << "'"
			          << "}" << std::endl;
#endif
		}

		void RtpPacket::Clear()
		{
			this->sendTransportId = "undefined";
			this->routerId        = "undefined";
			this->producerId      = "undefined";
			this->sendSeqNumber   = { 0 };
			this->dropped         = { false };
			this->dropReason      = { DropReason::NONE };
		}
	} // namespace RtcLogger
} // namespace RTC
