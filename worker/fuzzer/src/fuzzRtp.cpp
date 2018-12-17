#include "fuzzers.hpp"
#include "RTC/RtpPacket.hpp"

using namespace RTC;

namespace fuzzers
{
	void fuzzRtcp(const uint8_t* data, size_t len)
	{
		if (!RtpPacket::IsRtp(data, len))
			return;

		RtpPacket* packet = RtpPacket::Parse(data, len);

		if (!packet)
			return;

		// TODO: Add more checks here.

		delete packet;
	}
}
