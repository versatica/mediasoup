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
	static constexpr size_t ScoreHistogramLength{ 24 };

	/* Instance methods. */

	RtpStream::RtpStream(
	  RTC::RtpStream::Listener* listener, RTC::RtpStream::Params& params, uint8_t initialScore)
	  : listener(listener), params(params), score(initialScore)
	{
		MS_TRACE();
	}

	RtpStream::~RtpStream()
	{
		MS_TRACE();
	}

	void RtpStream::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Add params.
		this->params.FillJson(jsonObject["params"]);

		// Add score.
		jsonObject["score"] = this->score;

		// Add totalSourceLoss.
		jsonObject["totalSourceLoss"] = this->totalSourceLoss;

		// Add totalReportedLoss.
		jsonObject["totalReportedLoss"] = this->totalReportedLoss;
	}

	void RtpStream::FillJsonStats(json& jsonObject)
	{
		MS_TRACE();

		uint64_t now = DepLibUV::GetTime();

		jsonObject["timestamp"]            = now;
		jsonObject["ssrc"]                 = this->params.ssrc;
		jsonObject["kind"]                 = RtpCodecMimeType::type2String[this->params.mimeType.type];
		jsonObject["mimeType"]             = this->params.mimeType.ToString();
		jsonObject["packetCount"]          = this->transmissionCounter.GetPacketCount();
		jsonObject["byteCount"]            = this->transmissionCounter.GetBytes();
		jsonObject["bitrate"]              = this->transmissionCounter.GetRate(now);
		jsonObject["packetsLost"]          = this->packetsLost;
		jsonObject["fractionLost"]         = this->fractionLost;
		jsonObject["packetsDiscarded"]     = this->packetsDiscarded;
		jsonObject["packetsRetransmitted"] = this->packetsRetransmitted;
		jsonObject["packetsRepaired"]      = this->packetsRepaired;
		jsonObject["nackCount"]            = this->nackCount;
		jsonObject["nackPacketCount"]      = this->nackPacketCount;
		jsonObject["pliCount"]             = this->pliCount;
		jsonObject["firCount"]             = this->firCount;
		jsonObject["score"]                = this->score;

		if (!this->params.rid.empty())
			jsonObject["rid"] = this->params.rid;

		if (this->params.rtxSsrc)
			jsonObject["rtxSsrc"] = this->params.rtxSsrc;
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

		// Update highest seen RTP timestamp.
		if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(packet->GetTimestamp(), this->maxPacketTs))
		{
			this->maxPacketTs = packet->GetTimestamp();
			this->maxPacketMs = DepLibUV::GetTime();
		}

		return true;
	}

	void RtpStream::ResetScore(uint8_t score, bool notify)
	{
		MS_TRACE();

		this->totalSourceLoss   = 0;
		this->totalReportedLoss = 0;
		this->totalSentPackets  = 0;

		this->scores.clear();

		if (this->score != score)
		{
			this->score = score;

			// Notify the listener.
			if (notify)
				this->listener->OnRtpStreamScore(this, score);
		}
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

	void RtpStream::UpdateScore(uint8_t score)
	{
		MS_TRACE();

		// Add the score into the histogram.
		if (this->scores.size() == ScoreHistogramLength)
			this->scores.erase(this->scores.begin());

		auto previousScore = this->score;

		// Compute new effective score taking into accout entries in the histogram.
		this->scores.push_back(score);

		/*
		 * Scoring mechanism is a weighted average.
		 *
		 * The more recent the score is, the more weight it has.
		 * The oldest score has a weight of 1 and subsequent scores weight is
		 * increased by one sequentially.
		 *
		 * Ie:
		 * - scores: [1,2,3,4]
		 * - this->scores = ((1) + (2+2) + (3+3+3) + (4+4+4+4)) / 10 = 2.8 => 3
		 */

		size_t weight{ 0 };
		size_t samples{ 0 };
		size_t totalScore{ 0 };

		for (auto score : this->scores)
		{
			weight++;
			samples += weight;
			totalScore += weight * score;
		}

		// clang-tidy "thinks" that this can lead to division by zero but we are
		// smarter.
		// NOLINTNEXTLINE(clang-analyzer-core.DivideZero)
		this->score = static_cast<uint8_t>(std::round(totalScore / samples));

		// Call the listener if the global score has changed.
		if (this->score != previousScore)
		{
			MS_DEBUG_TAG(
			  score,
			  "[added score:%" PRIu8 ", previous computed score:%" PRIu8 ", new computed score:%" PRIu8
			  "] (calling listener)",
			  score,
			  previousScore,
			  this->score);

			this->listener->OnRtpStreamScore(this, this->score);
		}
		else
		{
#ifdef MS_LOG_DEV
			MS_DEBUG_TAG(
			  score,
			  "[added score:%" PRIu8 ", previous computed score:%" PRIu8 ", new computed score:%" PRIu8
			  "] (no change)",
			  score,
			  previousScore,
			  this->score);
#endif
		}
	}

	void RtpStream::PacketRetransmitted(RTC::RtpPacket* /*packet*/)
	{
		MS_TRACE();

		this->packetsRetransmitted++;
	}

	void RtpStream::PacketRepaired(RTC::RtpPacket* /*packet*/)
	{
		MS_TRACE();

		this->packetsRepaired++;
	}

	inline void RtpStream::InitSeq(uint16_t seq)
	{
		MS_TRACE();

		// Initialize/reset RTP counters.
		this->baseSeq = seq;
		this->maxSeq  = seq;
		this->badSeq  = RtpSeqMod + 1; // So seq == badSeq is false.
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

		jsonObject["cname"] = this->cname;

		if (this->rtxSsrc != 0)
		{
			jsonObject["rtxSsrc"]        = this->rtxSsrc;
			jsonObject["rtxPayloadType"] = this->rtxPayloadType;
		}

		jsonObject["useNack"]        = this->useNack;
		jsonObject["usePli"]         = this->usePli;
		jsonObject["useFir"]         = this->useFir;
		jsonObject["useInBandFec"]   = this->useInBandFec;
		jsonObject["useDtx"]         = this->useDtx;
		jsonObject["temporalLayers"] = this->temporalLayers;
	}
} // namespace RTC
