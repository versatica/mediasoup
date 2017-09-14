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
	static constexpr uint16_t MaxMisorder{ 300 };
	static constexpr uint32_t RtpSeqMod{ 1 << 16 };
	static constexpr uint16_t HealthCheckPeriod{ 1000 };

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

		static const Json::StaticString JsonStringBitRate{ "bitrate" };
		static const Json::StaticString JsonStringJitter{ "jitter" };
		static const Json::StaticString JsonStringMaxTimestamp{ "maxTimestamp" };
		static const Json::StaticString JsonStringParams{ "params" };
		static const Json::StaticString JsonStringPacketCount{ "packetCount" };
		static const Json::StaticString JsonStringOctetCount{ "octetCount" };
		static const Json::StaticString JsonStringRtt{ "rtt" };
		static const Json::StaticString JsonStringFractionLost{ "fractionLost" };
		static const Json::StaticString JsonStringTotalLost{ "totalLost" };

		Json::Value json(Json::objectValue);

		json[JsonStringBitRate]      = Json::UInt{ GetBitRate() };
		json[JsonStringJitter]       = Json::UInt{ this->jitter };
		json[JsonStringMaxTimestamp] = Json::UInt{ this->maxTimestamp };
		json[JsonStringParams]       = this->params.ToJson();
		json[JsonStringPacketCount]  = static_cast<Json::UInt>(this->counter.GetPacketCount());
		json[JsonStringOctetCount]   = static_cast<Json::UInt>(this->counter.GetBytes());
		json[JsonStringRtt]          = Json::UInt{ this->rtt };
		json[JsonStringFractionLost] = Json::UInt{ this->fractionLost };
		json[JsonStringTotalLost]    = Json::UInt{ this->totalLost };

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

		// Set the extended sequence number into the packet.
		packet->SetExtendedSequenceNumber(
		  this->cycles + static_cast<uint32_t>(packet->GetSequenceNumber()));

		// Update highest seen RTP timestamp.
		if (packet->GetTimestamp() > this->maxTimestamp)
			this->maxTimestamp = packet->GetTimestamp();

		return true;
	}

	uint32_t RtpStream::GetBitRate()
	{
		uint64_t now = DepLibUV::GetTime();

		return this->counter.GetRate(now);
	}

	void RtpStream::InitSeq(uint16_t seq)
	{
		MS_TRACE();

		// Initialize/reset RTP counters.
		this->baseSeq       = seq;
		this->maxSeq        = seq;
		this->badSeq        = RtpSeqMod + 1; // So seq == badSeq is false.
		this->cycles        = 0;
		this->receivedPrior = 0;
		this->expectedPrior = 0;
		this->maxTimestamp  = 0; // Also reset the highest seen RTP timestamp.

		// Call the OnInitSeq method of the child.
		OnInitSeq();
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

				return false;
			}
		}
		// Acceptable misorder.
		else
		{
			// Do nothing.
		}

		// Increase counters.
		this->counter.Update(packet);

		return true;
	}

	Json::Value RtpStream::Params::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringSsrc{ "ssrc" };
		static const Json::StaticString JsonStringPayloadType{ "payloadType" };
		static const Json::StaticString JsonStringMime{ "mime" };
		static const Json::StaticString JsonStringClockRate{ "clockRate" };
		static const Json::StaticString JsonStringUseNack{ "useNack" };
		static const Json::StaticString JsonStringUsePli{ "usePli" };

		Json::Value json(Json::objectValue);

		json[JsonStringSsrc]        = Json::UInt{ this->ssrc };
		json[JsonStringPayloadType] = Json::UInt{ this->payloadType };
		json[JsonStringMime]        = this->mime.ToString();
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
