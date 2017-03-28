#define MS_CLASS "RTC::NackGenerator"
// #define MS_LOG_DEV

#include "RTC/NackGenerator.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <bitset> // std::bitset()

namespace RTC
{
	/* Instance methods. */

	NackGenerator::NackGenerator(Listener* listener) :
		listener(listener)
	{
		MS_TRACE();

		// Set the timer.
		this->timer = new Timer(this);
	}

	NackGenerator::~NackGenerator()
	{
		MS_TRACE();

		// Close the timer.
		this->timer->Close();
	}

	void NackGenerator::ReceivePacket(RTC::RtpPacket* packet, uint32_t seq32)
	{
		MS_TRACE();

		// TODO: This is just a copy&paste of the previous NACK algorithm located in
		// the RtpStreamRecv.MayTriggerNack(). This must be refactorized.

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

		// Just received next expected seq, do nothing.
		if (diff_seq32 == 1)
			return;

		// If 16 or more packets was lost, ask for a full frame.
		if (diff_seq32 >= 16)
		{
			MS_DEBUG_DEV("full frame required");

			this->listener->onFullFrameRequired();

			// TODO: Now we should stop sending NACK for a while (ideally after a full keyframe
			// is received...).

			return;
		}

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

		MS_DEBUG_DEV("NACK required [seq:%" PRIu16 ", bitmask:" MS_UINT16_TO_BINARY_PATTERN "]",
			nack_seq, MS_UINT16_TO_BINARY(nack_bitmask));

		this->listener->onNackRequired(nack_seq, nack_bitmask);
	}

	inline
	void NackGenerator::onTimer(Timer* timer)
	{
		MS_TRACE();
	}
}
