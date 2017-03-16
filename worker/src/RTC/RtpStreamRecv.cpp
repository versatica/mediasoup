#define MS_CLASS "RTC::RtpStreamRecv"
// #define MS_LOG_DEV

#include "RTC/RtpStreamRecv.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <bitset> // std::bitset()

namespace RTC
{
	/* Instance methods. */

	RtpStreamRecv::RtpStreamRecv(Listener* listener, uint32_t ssrc, uint32_t clockRate, bool useNack) :
		RtpStream::RtpStream(ssrc, clockRate),
		listener(listener),
		useNack(useNack)
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

		static Json::Value null_data(Json::nullValue);
		static const Json::StaticString k_ssrc("ssrc");
		static const Json::StaticString k_clockRate("clockRate");
		static const Json::StaticString k_received("received");
		static const Json::StaticString k_maxTimestamp("maxTimestamp");
		static const Json::StaticString k_transit("transit");
		static const Json::StaticString k_jitter("jitter");

		Json::Value json(Json::objectValue);

		json[k_ssrc] = (Json::UInt)this->ssrc;
		json[k_clockRate] = (Json::UInt)this->clockRate;
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

		// May trigger a NACK to the sender.
		if (this->useNack)
			MayTriggerNack(packet);

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
		if (!this->clockRate)
			return;

		int transit = DepLibUV::GetTime() - (rtpTimestamp * 1000 / this->clockRate);
		int d = transit - this->transit;

		this->transit = transit;
		if (d < 0) d = -d;
		this->jitter += (1./16.) * ((double)d - this->jitter);
	}

	void RtpStreamRecv::MayTriggerNack(RTC::RtpPacket* packet)
	{
		uint32_t seq32 = (uint32_t)packet->GetSequenceNumber() + this->cycles;

		// If this is the first packet, just update last seen extended seq number.
		if (this->last_seq32 == 0)
		{
			this->last_seq32 = (seq32 != 0 ? seq32 : 1);
			return;
		}

		int32_t diff_seq32 = seq32 - this->last_seq32;

		// If the received seq is older than the last seen, ignore.
		if (diff_seq32 < 1)
			return;
		// Otherwise, update the last seen seq.
		else
			this->last_seq32 = seq32;

		// Just received next expected seq, do nothing else.
		if (diff_seq32 == 1)
			return;

		// Some packet(s) is/are missing, trigger a NACK.
		uint8_t nack_bitmask_count = std::min(diff_seq32 - 2, 16);
		uint32_t nack_seq32 = this->last_seq32 - nack_bitmask_count - 1;
		std::bitset<16> nack_bitset(0);

		for (uint8_t i = 0; i < nack_bitmask_count; ++i)
		{
			nack_bitset[i] = 1;
		}

		uint16_t nack_seq = (uint16_t)nack_seq32;
		uint16_t nack_bitmask = (uint16_t)nack_bitset.to_ulong();

		MS_DEBUG_TAG(rtcp, "NACK triggered [ssrc:%" PRIu32 ", seq:%" PRIu16 ", bitmask:" MS_UINT16_TO_BINARY_PATTERN "]",
			this->ssrc, nack_seq, MS_UINT16_TO_BINARY(nack_bitmask));

		this->listener->onNackRequired(this, nack_seq, nack_bitmask);
	}

	void RtpStreamRecv::onInitSeq()
	{
		this->last_seq32 = 0;
	}
}
