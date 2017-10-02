// DOC: https://tools.ietf.org/html/rfc3550#appendix-A.1

#define MS_CLASS "RTC::RtpStream"
// #define MS_LOG_DEV

#include "RTC/RtpStream.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint16_t MaxDropout{ 3000 };
	static constexpr uint16_t MaxMisorder{ 1500 };
	static constexpr uint32_t RtpSeqMod{ 1 << 16 };

	/* Instance methods. */

	RtpStream::RtpStream(RTC::RtpStream::Params& params) : params(params)
	{
		MS_TRACE();

		// Set the health check timer.
		this->healthCheckTimer = new Timer(this);

		// Run the timer.
		this->healthCheckTimer->Start(HealthCheckPeriod, HealthCheckPeriod);
	}

	RtpStream::~RtpStream()
	{
		MS_TRACE();

		// Close the health check timer.
		this->healthCheckTimer->Destroy();
	}

	Json::Value RtpStream::ToJson()
	{
		MS_TRACE();

		static const Json::StaticString JsonStringParams{ "params" };
		static const Json::StaticString JsonStringStats{ "stats" };

		Json::Value json(Json::objectValue);

		json[JsonStringParams] = this->params.ToJson();
		json[JsonStringStats]  = GetStats();

		return json;
	}

	Json::Value RtpStream::GetStats()
	{
		MS_TRACE();

		static const Json::StaticString JsonStringTimestamp{ "timestamp" };
		static const Json::StaticString JsonStringSsrc{ "ssrc" };
		static const Json::StaticString JsonStringMediaType{ "mediaType" };
		static const Json::StaticString JsonStringHealthy{ "healthy" };
		static const Json::StaticString JsonStringPacketCount{ "packetCount" };
		static const Json::StaticString JsonStringByteCount{ "byteCount" };
		static const Json::StaticString JsonStringBitRate{ "bitrate" };
		static const Json::StaticString JsonStringPacketsLost{ "packetsLost" };
		static const Json::StaticString JsonStringFractionLost{ "fractionLost" };
		static const Json::StaticString JsonStringJitter{ "jitter" };
		static const Json::StaticString JsonStringRtt{ "rtt" };
		static const Json::StaticString JsonStringPacketsDiscarded{ "packetsDiscarded" };
		static const Json::StaticString JsonStringPacketsRepaired{ "packetsRepaired" };
		static const Json::StaticString JsonStringFirCount{ "firCount" };
		static const Json::StaticString JsonStringPliCount{ "pliCount" };
		static const Json::StaticString JsonStringNackCount{ "nackCount" };
		static const Json::StaticString JsonStringSliCount{ "sliCount" };

		Json::Value json(Json::objectValue);
		uint64_t now = DepLibUV::GetTime();

		json[JsonStringTimestamp]   = Json::UInt64{ now };
		json[JsonStringSsrc]        = Json::UInt{ this->params.ssrc };
		json[JsonStringMediaType]   = RtpCodecMimeType::type2String[this->params.mimeType.type];
		json[JsonStringHealthy]     = this->healthy ? "true" : "false";
		json[JsonStringPacketCount] = static_cast<Json::UInt>(this->transmissionCounter.GetPacketCount());
		json[JsonStringByteCount]   = static_cast<Json::UInt>(this->transmissionCounter.GetBytes());
		json[JsonStringBitRate]     = Json::UInt{ this->transmissionCounter.GetRate(now) };
		json[JsonStringPacketsLost]      = Json::UInt{ this->packetsLost };
		json[JsonStringFractionLost]     = Json::UInt{ this->fractionLost };
		json[JsonStringJitter]           = Json::UInt{ this->jitter };
		json[JsonStringRtt]              = Json::UInt{ this->rtt };
		json[JsonStringPacketsDiscarded] = static_cast<Json::UInt>(this->packetsDiscarded);
		json[JsonStringPacketsRepaired]  = static_cast<Json::UInt>(this->packetsRepaired);
		json[JsonStringFirCount]         = static_cast<Json::UInt>(this->firCount);
		json[JsonStringPliCount]         = static_cast<Json::UInt>(this->pliCount);
		json[JsonStringNackCount]        = static_cast<Json::UInt>(this->nackCount);
		json[JsonStringSliCount]         = static_cast<Json::UInt>(this->sliCount);

		return json;
	}

	bool RtpStream::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq = packet->GetSequenceNumber();

		// If this is the first packet seen, initialize stuff.
		if (!this->started)
		{
			InitSeq(seq);

			this->started = true;
			this->maxSeq  = seq - 1;
		}

		// If not a valid packet ignore it.
		if (!UpdateSeq(packet))
		{
			MS_WARN_TAG(
			  rtp,
			  "invalid packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
			  packet->GetSsrc(),
			  packet->GetSequenceNumber());

			return false;
		}

		// Increase counters.
		this->transmissionCounter.Update(packet);

		// Update highest seen RTP timestamp.
		if (packet->GetTimestamp() > this->maxPacketTs)
		{
			this->maxPacketTs = packet->GetTimestamp();
			this->maxPacketMs = DepLibUV::GetTime();
		}

		return true;
	}

	uint32_t RtpStream::GetRate(uint64_t now)
	{
		return this->transmissionCounter.GetRate(now);
	}

	void RtpStream::ResetHealthCheckTimer(uint16_t timeout)
	{
		// Notify about next health status.
		this->notifyHealth = true;
		this->healthCheckTimer->Start(timeout, HealthCheckPeriod);
	}

	void RtpStream::InitSeq(uint16_t seq)
	{
		MS_TRACE();

		// Initialize/reset RTP counters.
		this->baseSeq     = seq;
		this->maxSeq      = seq;
		this->badSeq      = RtpSeqMod + 1; // So seq == badSeq is false.
		this->maxPacketTs = 0;             // Also reset the highest seen RTP timestamp.
	}

	bool RtpStream::UpdateSeq(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq    = packet->GetSequenceNumber();
		uint16_t udelta = seq - this->maxSeq;

		// If the new packet sequence number is greater than the max seen but not
		// "so much bigger", accept it.
		// NOTE: udelta also handles the case of a new cycle, this is:
		//    maxSeq:65536, seq:0 => udelta:1
		if (udelta < MaxDropout)
		{
			// In order, with permissible gap.
			if (seq < this->maxSeq)
			{
				// Sequence number wrapped: count another 64K cycle.
				this->cycles += RtpSeqMod;
			}

			this->maxSeq = seq;
		}
		// Too old packet received (older than the allowed misorder).
		// Or to new packet (more than acceptable dropout).
		else if (udelta <= RtpSeqMod - MaxMisorder)
		{
			// The sequence number made a very large jump. If two sequential packets
			// arrive, accept the latter.
			if (seq == this->badSeq)
			{
				// Two sequential packets. Assume that the other side restarted without
				// telling us so just re-sync (i.e., pretend this was the first packet).
				MS_WARN_TAG(
				  rtp,
				  "too bad sequence number, re-syncing RTP [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber());

				InitSeq(seq);
			}
			else
			{
				MS_WARN_TAG(
				  rtp,
				  "bad sequence number, ignoring packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber());

				this->badSeq = (seq + 1) & (RtpSeqMod - 1);
				this->packetsDiscarded++;

				return false;
			}
		}
		// Acceptable misorder.
		else
		{
			// Do nothing.
		}

		return true;
	}

	Json::Value RtpStream::Params::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringSsrc{ "ssrc" };
		static const Json::StaticString JsonStringPayloadType{ "payloadType" };
		static const Json::StaticString JsonStringMimeType{ "mimeType" };
		static const Json::StaticString JsonStringClockRate{ "clockRate" };
		static const Json::StaticString JsonStringUseNack{ "useNack" };
		static const Json::StaticString JsonStringUsePli{ "usePli" };

		Json::Value json(Json::objectValue);

		json[JsonStringSsrc]        = Json::UInt{ this->ssrc };
		json[JsonStringPayloadType] = Json::UInt{ this->payloadType };
		json[JsonStringMimeType]    = this->mimeType.ToString();
		json[JsonStringClockRate]   = Json::UInt{ this->clockRate };
		json[JsonStringUseNack]     = this->useNack;
		json[JsonStringUsePli]      = this->usePli;

		return json;
	}

	void RtpStream::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->healthCheckTimer)
		{
			this->CheckHealth();
		}
	}
} // namespace RTC
