#include "fuzzers.hpp"
#include "RTC/StunMessage.hpp"

using namespace RTC;

namespace fuzzers
{
	void fuzzStun(const uint8_t* data, size_t len)
	{
		if (!StunMessage::IsStun(data, len))
			return;

		StunMessage* msg = StunMessage::Parse(data, len);

		if (!msg)
			return;

		// TODO: Add more checks here.

		delete msg;
	}
}
