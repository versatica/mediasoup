// DOC: https://tools.ietf.org/html/rfc3550#appendix-A.1

#define MS_CLASS "RTC::RtpStream"
// #define MS_LOG_DEV

#include "RTC/RtpStream.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr uint32_t MinSequential{ 0 };
	static constexpr uint16_t MaxDropout{ 3000 };
	static constexpr uint16_t MaxMisorder{ 100 };
	static constexpr uint32_t RtpSeqMod{ 1 << 16 };

	/* Instance methods. */

	RtpStream::RtpStream(RTC::RtpStream::Params& params) : params(params)
	{
		MS_TRACE();
	}

	RtpStream::~RtpStream()
	{
		MS_TRACE();
	}

	bool RtpStream::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq = packet->GetSequenceNumber();

		// If this is the first packet seen, initialize stuff.
		if (!this->started)
		{
			InitSeq(seq);
			this->started   = true;
			this->maxSeq    = seq - 1;
			this->probation = MinSequential;
		}

		// If not a valid packet ignore it.
		if (!UpdateSeq(packet))
		{
			if (this->probation == 0u)
			{
				MS_WARN_TAG(
				    rtp,
				    "invalid packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]",
				    packet->GetSsrc(),
				    packet->GetSequenceNumber());
			}

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

	void RtpStream::InitSeq(uint16_t seq)
	{
		MS_TRACE();

		// Initialize/reset RTP counters.
		this->baseSeq       = seq;
		this->maxSeq        = seq;
		this->badSeq        = RtpSeqMod + 1; // So seq == badSeq is false.
		this->cycles        = 0;
		this->received      = 0;
		this->receivedPrior = 0;
		this->expectedPrior = 0;
		// Also reset the highest seen RTP timestamp.
		this->maxTimestamp = 0;

		// Call the OnInitSeq method of the child.
		OnInitSeq();
	}

	bool RtpStream::UpdateSeq(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		uint16_t seq    = packet->GetSequenceNumber();
		uint16_t udelta = seq - this->maxSeq;

		/*
		 * Source is not valid until MinSequential packets with
		 * sequential sequence numbers have been received.
		 */
		if (this->probation != 0u)
		{
			// Packet is in sequence.
			if (seq == this->maxSeq + 1)
			{
				this->probation--;
				this->maxSeq = seq;

				if (this->probation == 0)
				{
					InitSeq(seq);
					this->received++;

					return true;
				}
			}
			else
			{
				this->probation = MinSequential - 1;
				this->maxSeq    = seq;
			}

			return false;
		}
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
		else if (udelta <= RtpSeqMod - MaxMisorder)
		{
			// The sequence number made a very large jump.
			if (seq == this->badSeq)
			{
				/*
				 * Two sequential packets -- assume that the other side
				 * restarted without telling us so just re-sync
				 * (i.e., pretend this was the first packet).
				 */
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

		this->received++;

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
} // namespace RTC
