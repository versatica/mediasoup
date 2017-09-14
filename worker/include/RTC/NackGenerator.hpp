#ifndef MS_RTC_NACK_GENERATOR_HPP
#define MS_RTC_NACK_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"
#include <map>
#include <vector>

namespace RTC
{
	class NackGenerator : public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers) = 0;
			virtual void OnNackGeneratorKeyFrameRequired()                                    = 0;
		};

	private:
		struct NackInfo
		{
			NackInfo(){};
			explicit NackInfo(uint32_t seq32, uint32_t sendAtSeqNum);

			uint32_t seq32{ 0 };
			uint32_t sendAtSeqNum{ 0 };
			uint64_t sentAtTime{ 0 };
			uint8_t retries{ 0 };
		};

		enum class NackFilter
		{
			SEQ,
			TIME
		};

	public:
		explicit NackGenerator(Listener* listener);
		~NackGenerator() override;

		void ReceivePacket(RTC::RtpPacket* packet);

	private:
		void AddPacketsToNackList(uint32_t seq32Start, uint32_t seq32End);
		void RemoveFromNackListOlderThan(RTC::RtpPacket* packet);
		std::vector<uint16_t> GetNackBatch(NackFilter filter);
		void MayRunTimer() const;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		Timer* timer{ nullptr };
		// Others.
		std::map<uint32_t, NackInfo> nackList;
		bool started{ false };
		uint32_t lastSeq32{ 0 }; // Extended seq number of last valid packet.
		uint32_t rtt{ 0 };       // Round trip time (ms).
	};

	// Inline instance methods.

	inline NackGenerator::NackInfo::NackInfo(uint32_t seq32, uint32_t sendAtSeqNum)
	  : seq32(seq32), sendAtSeqNum(sendAtSeqNum)
	{
	}
} // namespace RTC

#endif
