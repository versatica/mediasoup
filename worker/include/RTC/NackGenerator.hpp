#ifndef MS_RTC_NACK_GENERATOR_HPP
#define MS_RTC_NACK_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"

namespace RTC
{
	class NackGenerator :
		public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onNackRequired(uint16_t seq, uint16_t bitmask) = 0;
			virtual void onFullFrameRequired() = 0;
		};

	public:
		explicit NackGenerator(Listener* listener);
		~NackGenerator();

		void ReceivePacket(RTC::RtpPacket* packet, uint32_t seq32);

	/* Pure virtual methods inherited from Timer::Listener. */
	public:
		virtual void onTimer(Timer* timer) override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Allocated by this.
		Timer* timer = nullptr;
		// Others.
		uint32_t last_seq32 = 0; // Extended seq number of last valid packet.
	};
}

#endif
