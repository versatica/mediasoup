#ifndef MS_RTC_RTP_STREAM_HPP
#define MS_RTC_RTP_STREAM_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include <json/json.h>

namespace RTC
{
	class RtpStream
	{
	public:
		explicit RtpStream(uint32_t ssrc, uint32_t clockRate);
		virtual ~RtpStream();

		virtual Json::Value toJson() const = 0;
		uint32_t GetSsrc();
		virtual bool ReceivePacket(RTC::RtpPacket* packet);

	private:
		void InitSeq(uint16_t seq);
		bool UpdateSeq(uint16_t seq);

	/* Pure virtual methods that must be implemented by the subclass. */
	protected:
		virtual void onInitSeq() = 0;

	protected:
		// Given as argument.
		uint32_t ssrc = 0;
		uint32_t clockRate = 0;
		bool started = false; // Whether at least a RTP packet has been received.
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

	/* Inline instance methods. */

	inline
	uint32_t RtpStream::GetSsrc()
	{
		return this->ssrc;
	}
}

#endif
