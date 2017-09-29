#ifndef MS_RTC_NACK_GENERATOR_HPP
#define MS_RTC_NACK_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SeqManager.hpp"
#include "handles/Timer.hpp"
#include <map>
#include <set>
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
			explicit NackInfo(uint16_t seq, uint16_t sendAtSeq);

			uint16_t seq{ 0 };
			uint16_t sendAtSeq{ 0 };
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

		bool ReceivePacket(RTC::RtpPacket* packet);
		size_t GetNackListLength() const;

	private:
		void CleanOldNackItems(uint16_t seq);
		void AddPacketsToNackList(uint16_t seqStart, uint16_t seqEnd);
		void RemoveNackItemsUntilKeyFrame();
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
		std::map<uint16_t, NackInfo, SeqManager<uint16_t>::SeqLowerThan> nackList;
		// This set is just supposed to hold zero or one entries.
		std::set<uint16_t, SeqManager<uint16_t>::SeqLowerThan> keyFrameList;
		bool started{ false };
		uint16_t lastSeq{ 0 }; // Seq number of last valid packet.
		uint32_t rtt{ 0 };     // Round trip time (ms).
	};

	// Inline instance methods.

	inline NackGenerator::NackInfo::NackInfo(uint16_t seq, uint16_t sendAtSeq)
	  : seq(seq), sendAtSeq(sendAtSeq)
	{
	}

	inline size_t NackGenerator::GetNackListLength() const
	{
		return this->nackList.size();
	}
} // namespace RTC

#endif
