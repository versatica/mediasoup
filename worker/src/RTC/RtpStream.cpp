// DOC: https://tools.ietf.org/html/rfc3550#appendix-A.1

#define MS_CLASS "RTC::RtpStream"
// #define MS_LOG_DEV

#include "RTC/RtpStream.hpp"
#include "Logger.hpp"

#define MIN_SEQUENTIAL 0
#define MAX_DROPOUT 3000
#define MAX_MISORDER 100
#define RTP_SEQ_MOD (1<<16)

namespace RTC
{
	/* Instance methods. */

	RtpStream::RtpStream(RTC::RtpStream::Params& params) :
		params(params)
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
			this->started = true;

			InitSeq(seq);
			this->max_seq = seq - 1;
			this->probation = MIN_SEQUENTIAL;
		}

		// If not a valid packet ignore it.
		if (!UpdateSeq(seq))
		{
			if (!this->probation)
			{
				MS_WARN_TAG(rtp, "invalid packet [ssrc:%" PRIu32 ", seq:%" PRIu16 "]", packet->GetSsrc(), packet->GetSequenceNumber());
			}

			return false;
		}

		// Update highest seen RTP timestamp.
		if (packet->GetTimestamp() > this->max_timestamp)
			this->max_timestamp = packet->GetTimestamp();

		return true;
	}

	void RtpStream::InitSeq(uint16_t seq)
	{
		MS_TRACE();

		// Initialize/reset RTP counters.
		this->base_seq = seq;
		this->max_seq = seq;
		this->bad_seq = RTP_SEQ_MOD + 1; // So seq == bad_seq is false.
		this->cycles = 0;
		this->received = 0;
		this->received_prior = 0;
		this->expected_prior = 0;
		// Also reset the highest seen RTP timestamp.
		this->max_timestamp = 0;

		// Call the onInitSeq method of the child.
		onInitSeq();
	}

	bool RtpStream::UpdateSeq(uint16_t seq)
	{
		MS_TRACE();

		uint16_t udelta = seq - this->max_seq;

		/*
		 * Source is not valid until MIN_SEQUENTIAL packets with
		 * sequential sequence numbers have been received.
		 */
		if (this->probation)
		{
			// Packet is in sequence.
			if (seq == this->max_seq + 1)
			{
				this->probation--;
				this->max_seq = seq;

				if (this->probation == 0)
				{
					InitSeq(seq);
					this->received++;

					return true;
				}
			}
			else
			{
				this->probation = MIN_SEQUENTIAL - 1;
				this->max_seq = seq;
			}

			return false;
		}
		else if (udelta < MAX_DROPOUT)
		{
			// In order, with permissible gap.
			if (seq < this->max_seq)
			{
				// Sequence number wrapped: count another 64K cycle.
				this->cycles += RTP_SEQ_MOD;
			}

			this->max_seq = seq;
		}
		else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER)
		{
			// The sequence number made a very large jump.
			if (seq == this->bad_seq)
			{
				/*
				 * Two sequential packets -- assume that the other side
				 * restarted without telling us so just re-sync
				 * (i.e., pretend this was the first packet).
				 */
				InitSeq(seq);
			}
			else
			{
				this->bad_seq = (seq + 1) & (RTP_SEQ_MOD - 1);

				return false;
			}
		}
		else
		{
			// Duplicate or reordered packet.
			// NOTE: This would never happen because libsrtp rejects duplicated packets.
		}

		this->received++;

		return true;
	}

	Json::Value RtpStream::Params::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_mime("mime");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_useNack("useNack");
		static const Json::StaticString k_absSendTimeId("absSendTimeId");

		Json::Value json(Json::objectValue);

		json[k_ssrc] = (Json::UInt)this->ssrc;
		json[k_payloadType] = (Json::UInt)this->payloadType;
		json[k_mime] = this->mime.name;
		json[k_clockRate] = (Json::UInt)this->clockRate;
		json[k_useNack] = this->useNack;
		json[k_absSendTimeId] = (Json::UInt)this->absSendTimeId;

		return json;
	}
}
