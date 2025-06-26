#define MS_CLASS "RTC::RtcLogger"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/RtcLogger.hpp"
#include "Logger.hpp"

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

			// TODO: Here we are using std::cout() which means that it's directly
			// written to stdout. When in Node, it means that Worker.ts captures it
			// and prints it ONLY if DEBUG env contains "Worker", and it prefixes the
			// log with "(stdout)". We should move to MS_DUMP() or MS_DUMP_CLEAN().

			std::cout << "{";
			std::cout << "\"timestamp\": " << this->timestamp;

			if (!this->recvTransportId.empty())
			{
				std::cout << R"(, "recvTransportId": ")" << this->recvTransportId << "\"";
			}
			if (!this->sendTransportId.empty())
			{
				std::cout << R"(, "sendTransportId": ")" << this->sendTransportId << "\"";
			}
			if (!this->routerId.empty())
			{
				std::cout << R"(, "routerId": ")" << this->routerId << "\"";
			}
			if (!this->producerId.empty())
			{
				std::cout << R"(, "producerId": ")" << this->producerId << "\"";
			}
			if (!this->consumerId.empty())
			{
				std::cout << R"(, "consumerId": ")" << this->consumerId << "\"";
			}

			std::cout << ", \"recvRtpTimestamp\": " << this->recvRtpTimestamp;
			std::cout << ", \"sendRtpTimestamp\": " << this->sendRtpTimestamp;
			std::cout << ", \"recvSeqNumber\": " << this->recvSeqNumber;
			std::cout << ", \"sendSeqNumber\": " << this->sendSeqNumber;
			std::cout << ", \"discarded\": " << (this->discarded ? "true" : "false");
			std::cout << ", \"discardReason\": '" << discardReason2String[this->discardReason] << "'";
			std::cout << "}" << std::endl;
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
