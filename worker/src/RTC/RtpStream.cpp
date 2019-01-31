// DOC: https://tools.ietf.org/html/rfc3550#appendix-A.1

#define MS_CLASS "RTC::RtpStream"
// #define MS_LOG_DEV

#include "RTC/RtpStream.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/SeqManager.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint16_t MaxDropout{ 3000 };
	static constexpr uint16_t MaxMisorder{ 1500 };
	static constexpr uint32_t RtpSeqMod{ 1 << 16 };

	/* Instance methods. */

	RtpStream::RtpStream(RTC::RtpStream::Listener* listener, RTC::RtpStream::Params& params)
	  : listener(listener), params(params)
	{
		MS_TRACE();

		// Set the status check timer.
		this->rtcpReportCheckTimer = new Timer(this);
	}

	RtpStream::~RtpStream()
	{
		MS_TRACE();

		// Close the status check timer.
		delete this->rtcpReportCheckTimer;
	}

	void RtpStream::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add params.
		this->params.FillJson(jsonObject["params"]);

		// Add score.
		jsonObject["score"] = this->rtpMonitor->GetScore();
	}

	void RtpStream::FillJsonStats(json& jsonObject)
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		jsonObject["timestamp"]          = now;
		jsonObject["ssrc"]               = this->params.ssrc;
		jsonObject["kind"]               = RtpCodecMimeType::type2String[this->params.mimeType.type];
		jsonObject["mimeType"]           = this->params.mimeType.ToString();
		jsonObject["packetCount"]        = this->transmissionCounter.GetPacketCount();
		jsonObject["byteCount"]          = this->transmissionCounter.GetBytes();
		jsonObject["bitrate"]            = this->transmissionCounter.GetRate(now);
		jsonObject["packetsLost"]        = this->packetsLost;
		jsonObject["fractionLost"]       = this->fractionLost;
		jsonObject["packetsDiscarded"]   = this->packetsDiscarded;
		jsonObject["packetsRepaired"]    = this->packetsRepaired;
		jsonObject["nackCount"]          = this->nackCount;
		jsonObject["nackRtpPacketCount"] = this->nackRtpPacketCount;
		jsonObject["pliCount"]           = this->pliCount;
		jsonObject["firCount"]           = this->firCount;

		// Add score.
		jsonObject["score"] = this->rtpMonitor->GetScore();
	}

	bool RtpStream::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq = packet->GetSequenceNumber();

		// If this is the first packet seen, initialize stuff.
		if (!this->started)
		{
			InitSeq(seq);

			this->started     = true;
			this->maxSeq      = seq - 1;
			this->maxPacketTs = packet->GetTimestamp();
			this->maxPacketMs = DepLibUV::GetTime();
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
		if (SeqManager<uint32_t>::IsSeqHigherThan(packet->GetTimestamp(), this->maxPacketTs))
		{
			this->maxPacketTs = packet->GetTimestamp();
			this->maxPacketMs = DepLibUV::GetTime();
		}

		return true;
	}

	void RtpStream::InitSeq(uint16_t seq)
	{
		MS_TRACE();

		// Initialize/reset RTP counters.
		this->baseSeq = seq;
		this->maxSeq  = seq;
		this->badSeq  = RtpSeqMod + 1; // So seq == badSeq is false.
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

				this->maxPacketTs = packet->GetTimestamp();
				this->maxPacketMs = DepLibUV::GetTime();
			}
			else
			{
				MS_WARN_TAG(
				  rtp,
				  "bad sequence number, ignoring packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				  packet->GetSsrc(),
				  packet->GetSequenceNumber());

				this->badSeq = (seq + 1) & (RtpSeqMod - 1);

				// Packet discarded due to late or early arriving.
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

	void RtpStream::Params::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		jsonObject["ssrc"]        = this->ssrc;
		jsonObject["payloadType"] = this->payloadType;
		jsonObject["mimeType"]    = this->mimeType.ToString();
		jsonObject["clockRate"]   = this->clockRate;

		if (!this->rid.empty())
			jsonObject["rid"] = this->rid;

		if (this->rtxSsrc != 0)
		{
			jsonObject["rtxSsrc"]        = this->rtxSsrc;
			jsonObject["rtxPayloadType"] = this->rtxPayloadType;
		}

		jsonObject["useNack"] = this->useNack;
		jsonObject["usePli"]  = this->usePli;
		jsonObject["useFir"]  = this->useFir;
	}
} // namespace RTC
