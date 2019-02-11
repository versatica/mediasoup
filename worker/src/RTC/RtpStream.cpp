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
	static constexpr size_t MaxRepairedPacketRetransmission{ 2 };
	static constexpr size_t MaxRepairedPacketsLength{ 1000 };
	static constexpr size_t ScoreHistogramLength{ 8 };

	/* Instance methods. */

	RtpStream::RtpStream(RTC::RtpStream::Listener* listener, RTC::RtpStream::Params& params)
	  : listener(listener), params(params)
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
		jsonObject["score"]              = this->score;
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
		if (RTC::SeqManager<uint32_t>::IsSeqHigherThan(packet->GetTimestamp(), this->maxPacketTs))
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

	void RtpStream::UpdateScore(RTC::RTCP::ReceiverReport* report)
	{
		MS_TRACE();

		// Calculate packet loss reported since last RR.
		auto previousTotalReportedLoss = this->totalReportedLoss;

		this->totalReportedLoss = report->GetTotalLost();

		auto reportedLoss = this->totalReportedLoss - previousTotalReportedLoss;

		if (reportedLoss < 0)
			reportedLoss = 0;

		// Calculate source packet loss since last RR.
		auto previousTotalSourceLoss = this->totalSourceLoss;
		size_t expectedPackets       = this->cycles + this->maxSeq - this->baseSeq + 1;

		this->totalSourceLoss =
		  static_cast<int32_t>(expectedPackets - this->transmissionCounter.GetPacketCount());

		auto sourceLoss = this->totalSourceLoss - previousTotalSourceLoss;

		if (sourceLoss < 0)
			sourceLoss = 0;

		// Calculate effective loss since last report.
		size_t currentLoss;

		if (reportedLoss < sourceLoss)
			currentLoss = 0;
		else
			currentLoss = reportedLoss - sourceLoss;

		// Calculate repaired packets.
		// TODO: This may have to be different in RtpStreamRecv and RtpStreamSend.
		size_t repairedPacketCount{ 0 };

		for (auto& kv : this->mapRepairedPackets)
		{
			if (kv.second <= MaxRepairedPacketRetransmission)
				repairedPacketCount++;
		}

		// Reset repaired packets map.
		this->mapRepairedPackets.clear();

		// Calculate packets sent since last RR.
		auto previousTotalSentPackets = this->totalSentPackets;

		this->totalSentPackets = this->transmissionCounter.GetPacketCount();

		auto sentPackets = this->totalSentPackets - previousTotalSentPackets;

		// Nothing to do.
		if (sentPackets == 0)
			return;

		// There cannot be more loss than sent packets.
		if (currentLoss > sentPackets)
			currentLoss = sentPackets;

		// There cannot be more repaired than sent packets.
		if (repairedPacketCount > sentPackets)
			repairedPacketCount = sentPackets;

		float lossPercentage     = currentLoss * 100 / sentPackets;
		float repairedPercentage = repairedPacketCount * 100 / sentPackets;

		/*
		 * Calculate score. Starting from a score of 100:
		 *
		 * - Each loss porcentual point has a weight of LossPercentageWeight.
		 * - Each repaired porcentual point has a  weight of RepairedPercentageWeight.
		 */

		float base100Score{ 100 };

		base100Score -= lossPercentage * 1.0f;
		base100Score += repairedPercentage * 0.5f;

		// Get base 10 score.
		auto score = static_cast<uint8_t>(std::lround(base100Score / 10));

#ifdef MS_LOG_DEV
		MS_DEBUG_TAG(
		  rtp,
		  "[sentPackets:%zu, currentLoss:%zu, totalSourceLoss:%" PRIi32 ", totalReportedLoss:%" PRIi32
		  ", repairedPacketCount:%zu, lossPercentage:%f, repairedPercentage:%f, score:%" PRIu8 "]",
		  sentPackets,
		  currentLoss,
		  this->totalSourceLoss,
		  this->totalReportedLoss,
		  repairedPacketCount,
		  lossPercentage,
		  repairedPercentage,
		  score);

		report->Dump();
#endif

		// Add the score into the histogram.
		if (this->scores.size() == ScoreHistogramLength)
			this->scores.erase(this->scores.begin());

		this->scores.push_back(score);

		// Compute new effective score taking into accout entries in the histogram.
		auto previousScore = this->score;

		if (!this->scores.empty())
		{
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

			this->score = static_cast<uint8_t>(std::round(totalScore / samples));
		}
		else
		{
			this->score = 0;
		}

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
			MS_DEBUG_TAG(
			  score, "[added score:%" PRIu8 ", computed score:%" PRIu8 "] (no change)", score, this->score);
		}
	}

	void RtpStream::PacketRepaired(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		this->retransmissionCounter.Update(packet);
		this->packetsRepaired++;

		if (this->mapRepairedPackets.size() == MaxRepairedPacketsLength)
			this->mapRepairedPackets.erase(this->mapRepairedPackets.begin());

		this->mapRepairedPackets[packet->GetSequenceNumber()]++;
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

		jsonObject["useNack"] = this->useNack;
		jsonObject["usePli"]  = this->usePli;
		jsonObject["useFir"]  = this->useFir;
	}
} // namespace RTC
