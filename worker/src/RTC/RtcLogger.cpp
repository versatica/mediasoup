#define MS_CLASS "RTC::RtcLogger"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtcLogger.hpp"
#include "Logger.hpp"
#include <sstream>

namespace RTC
{
	namespace RtcLogger
	{
		// clang-format off
		absl::flat_hash_map<RtpPacket::DiscardReason, std::string> RtpPacket::discardReason2String = {
			{ RtpPacket::DiscardReason::NONE,                                    "None"                               },
			{ RtpPacket::DiscardReason::PRODUCER_NOT_FOUND,                      "ProducerNotFound"                   },
			{ RtpPacket::DiscardReason::RECV_RTP_STREAM_NOT_FOUND,               "RecvRtpStreamNotFound"              },
			{ RtpPacket::DiscardReason::RECV_RTP_STREAM_DISCARDED,               "RecvRtpStreamDiscarded"             },
			{ RtpPacket::DiscardReason::RECV_RTP_RTX_STREAM_DISCARDED,           "RecvRtpRtxStreamDiscarded"          },
			{ RtpPacket::DiscardReason::CONSUMER_INACTIVE,                       "ConsumerInactive"                   },
			{ RtpPacket::DiscardReason::INVALID_TARGET_LAYER,                    "InvalidTargetLayer"                 },
			{ RtpPacket::DiscardReason::UNSUPPORTED_PAYLOAD_TYPE,                "UnsupportedPayloadType"             },
			{ RtpPacket::DiscardReason::NOT_A_KEYFRAME,                          "NotAKeyframe"                       },
			{ RtpPacket::DiscardReason::EMPTY_PAYLOAD,                           "EmptyPayload"                       },
			{ RtpPacket::DiscardReason::SPATIAL_LAYER_MISMATCH,                  "SpatialLayerMismatch"               },
			{ RtpPacket::DiscardReason::PACKET_PREVIOUS_TO_SPATIAL_LAYER_SWITCH, "PacketPreviousToSpatialLayerSwitch" },
			{ RtpPacket::DiscardReason::DROPPED_BY_CODEC,                        "DroppedByCodec"                     },
			{ RtpPacket::DiscardReason::TOO_HIGH_TIMESTAMP_EXTRA_NEEDED,         "TooHighTimestampExtraNeeded"},
			{ RtpPacket::DiscardReason::SEND_RTP_STREAM_DISCARDED, "SendRtpStreamDiscarded"}
		};
		// clang-format on

		void RtpPacket::Sent()
		{
			MS_TRACE();

			this->discarded = false;

			Log();
			Clear();
		}

		void RtpPacket::Discarded(DiscardReason discardReason)
		{
			MS_TRACE();

			this->discarded     = true;
			this->discardReason = discardReason;

			Log();
			Clear();
		}

		void RtpPacket::Log() const
		{
			MS_TRACE();

			std::stringstream ss;

			ss << "{";
			ss << "\"timestamp\": " << this->timestamp;

			if (!this->recvTransportId.empty())
			{
				ss << R"(, "recvTransportId": ")" << this->recvTransportId << "\"";
			}
			if (!this->sendTransportId.empty())
			{
				ss << R"(, "sendTransportId": ")" << this->sendTransportId << "\"";
			}
			if (!this->routerId.empty())
			{
				ss << R"(, "routerId": ")" << this->routerId << "\"";
			}
			if (!this->producerId.empty())
			{
				ss << R"(, "producerId": ")" << this->producerId << "\"";
			}
			if (!this->consumerId.empty())
			{
				ss << R"(, "consumerId": ")" << this->consumerId << "\"";
			}

			ss << ", \"recvRtpTimestamp\": " << this->recvRtpTimestamp;
			ss << ", \"sendRtpTimestamp\": " << this->sendRtpTimestamp;
			ss << ", \"recvSeqNumber\": " << this->recvSeqNumber;
			ss << ", \"sendSeqNumber\": " << this->sendSeqNumber;
			ss << ", \"discarded\": " << (this->discarded ? "true" : "false");
			ss << ", \"discardReason\": '" << discardReason2String[this->discardReason] << "'";
			ss << "}";

			MS_DUMP_CLEAN(0, "%s", ss.str().c_str());
		}

		void RtpPacket::Clear()
		{
			MS_TRACE();

			this->sendTransportId = {};
			this->routerId        = {};
			this->producerId      = {};
			this->sendSeqNumber   = { 0 };
			this->discarded       = { false };
			this->discardReason   = { DiscardReason::NONE };
		}
	} // namespace RtcLogger
} // namespace RTC
