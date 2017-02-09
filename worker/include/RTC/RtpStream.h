#ifndef MS_RTC_RTP_STREAM_H
#define MS_RTC_RTP_STREAM_H

#include "common.h"
#include "RTC/RtpPacket.h"

namespace RTC
{
	class RtpStream
	{
	public:
		explicit RtpStream(uint32_t clockRate);
		virtual ~RtpStream();

		virtual bool ReceivePacket(RTC::RtpPacket* packet);

	private:
		void InitSeq(uint16_t seq);
		bool UpdateSeq(uint16_t seq);

	protected:
		// Given as argument.
		uint32_t clockRate = 0;
		bool started = false; // Whether at least a RTP packet has been received.
		uint32_t last_sr_timestamp = 0; // The middle 32 bits out of 64 in the NTP timestamp received in the most recent sender report.
		uint64_t last_sr_received = 0; // Wallclock time representing the most recent sender report arrival.
		// https://tools.ietf.org/html/rfc3550#appendix-A.1 stuff.
		uint16_t max_seq = 0; // Highest seq. number seen.
		uint32_t cycles = 0; // Shifted count of seq. number cycles.
		uint32_t base_seq = 0; // Base seq number.
		uint32_t bad_seq = 0; // Last 'bad' seq number + 1.
		uint32_t probation = 0; // Seq. packets till source is valid.
		uint32_t received = 0; // Packets received.
		uint32_t expected_prior = 0; // Packet expected at last interval.
		uint32_t received_prior = 0; // Packet received at last interval.
		// Others.
		uint32_t max_timestamp = 0; // Highest timestamp seen.
	};
}

#endif
