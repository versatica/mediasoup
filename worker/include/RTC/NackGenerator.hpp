#ifndef MS_RTC_NACK_GENERATOR_HPP
#define MS_RTC_NACK_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"
#include <map>
#include <vector>

namespace RTC
{
	class NackGenerator :
		public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onNackRequired(const std::vector<uint16_t>& seq_numbers) = 0;
			virtual void onFullFrameRequired() = 0;
		};

	private:
		struct NackInfo
		{
			NackInfo() {};
			explicit NackInfo(uint32_t seq32);

			uint32_t seq32 = 0;
			uint64_t sent_at_time = 0;
			uint8_t retries = 0;
		};

		enum class NackFilter
		{
			SEQ,
			TIME
		};

	public:
		explicit NackGenerator(Listener* listener);
		~NackGenerator();

		void ReceivePacket(RTC::RtpPacket* packet);

	private:
		void AddPacketsToNackList(uint32_t seq32_start, uint32_t seq32_end);
		std::vector<uint16_t> GetNackBatch(NackFilter filter);
		void MayRunTimer() const;

	/* Pure virtual methods inherited from Timer::Listener. */
	public:
		virtual void onTimer(Timer* timer) override;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		// Allocated by this.
		Timer* timer = nullptr;
		// Others.
		std::map<uint32_t, NackInfo> nack_list;
		bool started = false;
		uint32_t last_seq32 = 0; // Extended seq number of last valid packet.
		uint32_t rtt = 0; // Round trip time (ms).
	};

	// Inline instance methods.

	inline
	NackGenerator::NackInfo::NackInfo(uint32_t seq32) :
		seq32(seq32)
	{}
}

#endif
