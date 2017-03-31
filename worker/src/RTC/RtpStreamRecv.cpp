#define MS_CLASS "RTC::RtpStreamRecv"
// #define MS_LOG_DEV

#include "RTC/RtpStreamRecv.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	RtpStreamRecv::RtpStreamRecv(Listener* listener, RTC::RtpStream::Params& params) :
		RtpStream::RtpStream(params),
		listener(listener)
	{
		MS_TRACE();
	}

	RtpStreamRecv::~RtpStreamRecv()
	{
		MS_TRACE();
	}

	Json::Value RtpStreamRecv::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString k_params("params");
		static const Json::StaticString k_received("received");
		static const Json::StaticString k_maxTimestamp("maxTimestamp");
		static const Json::StaticString k_transit("transit");
		static const Json::StaticString k_jitter("jitter");

		Json::Value json(Json::objectValue);

		json[k_params] = this->params.toJson();
		json[k_received] = (Json::UInt)this->received;
		json[k_maxTimestamp] = (Json::UInt)this->max_timestamp;
		json[k_transit] = (Json::UInt)this->transit;
		json[k_jitter] = (Json::UInt)this->jitter;

		return json;
	}

	bool RtpStreamRecv::ReceivePacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Call the parent method.
		if (!RtpStream::ReceivePacket(packet))
			return false;

		// Calculate Jitter.
		CalculateJitter(packet->GetTimestamp());

		// Set RTP header extension ids.
		if (this->params.absSendTimeId)
		{
			packet->AddExtensionMapping(
				RtpHeaderExtensionUri::Type::ABS_SEND_TIME, this->params.absSendTimeId);
		}

		// Pass the packet to the NackGenerator.
		if (this->params.useNack)
			this->nackGenerator->ReceivePacket(packet);

		return true;
	}

	RTC::RTCP::ReceiverReport* RtpStreamRecv::GetRtcpReceiverReport()
	{
		RTC::RTCP::ReceiverReport* report = new RTC::RTCP::ReceiverReport();

		// Calculate Packets Expected and Lost.
		uint32_t expected = (this->cycles + this->max_seq) - this->base_seq + 1;
		int32_t total_lost = expected - this->received;

		report->SetTotalLost(total_lost);

		// Calculate Fraction Lost.
		uint32_t expected_interval = expected - this->expected_prior;

		this->expected_prior = expected;

		uint32_t received_interval = this->received - this->received_prior;

		this->received_prior = this->received;

		int32_t lost_interval = expected_interval - received_interval;
		uint8_t fraction_lost;

		if (expected_interval == 0 || lost_interval <= 0)
			fraction_lost = 0;
		else
			fraction_lost = (lost_interval << 8) / expected_interval;

		report->SetFractionLost(fraction_lost);

		// Fill the rest of the report.
		report->SetLastSeq((uint32_t)this->max_seq + this->cycles);
		report->SetJitter(this->jitter);

		if (this->last_sr_received)
		{
			// Get delay in milliseconds.
			uint32_t delayMs = (DepLibUV::GetTime() - this->last_sr_received);

			// Express delay in units of 1/65536 seconds.
			uint32_t dlsr = (delayMs / 1000) << 16;
			dlsr |= (uint32_t)((delayMs % 1000) * 65536 / 1000);

			report->SetDelaySinceLastSenderReport(dlsr);
			report->SetLastSenderReport(this->last_sr_timestamp);
		}
		else
		{
			report->SetDelaySinceLastSenderReport(0);
			report->SetLastSenderReport(0);
		}

		return report;
	}

	void RtpStreamRecv::ReceiveRtcpSenderReport(RTC::RTCP::SenderReport* report)
	{
		this->last_sr_received = DepLibUV::GetTime();
		this->last_sr_timestamp = report->GetNtpSec() << 16;
		this->last_sr_timestamp += report->GetNtpFrac() >> 16;
	}

	void RtpStreamRecv::CalculateJitter(uint32_t rtpTimestamp)
	{
		if (!this->params.clockRate)
			return;

		int transit = DepLibUV::GetTime() - (rtpTimestamp * 1000 / this->params.clockRate);
		int d = transit - this->transit;

		this->transit = transit;
		if (d < 0) d = -d;
		this->jitter += (1./16.) * ((double)d - this->jitter);
	}

	void RtpStreamRecv::onInitSeq()
	{
		MS_TRACE();

		// Reset NackGenerator.
		if (this->params.useNack)
			this->nackGenerator.reset(new RTC::NackGenerator(this));
	}

	void RtpStreamRecv::onNackRequired(const std::vector<uint16_t>& seq_numbers)
	{
		MS_TRACE();

		MS_ASSERT(this->params.useNack, "NACK required but not supported");

		MS_DEBUG_TAG(rtx,
			"RTP retransmission required [ssrc:%" PRIu32 ", first_seq:%" PRIu16 ", num_packets:%zu]",
			this->params.ssrc, seq_numbers[0], seq_numbers.size());

		this->listener->onNackRequired(this, seq_numbers);
	}

	void RtpStreamRecv::onFullFrameRequired()
	{
		MS_TRACE();

		if (!this->params.usePli)
		{
			MS_WARN_TAG(rtx, "PLI required but not supported by the endpoint");
			return;
		}

		MS_DEBUG_TAG(rtx, "PLI triggered [ssrc:%" PRIu32 "]", this->params.ssrc);

		this->listener->onPliRequired(this);
	}
}
