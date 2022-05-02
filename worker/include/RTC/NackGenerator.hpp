#ifndef MS_RTC_NACK_GENERATOR_HPP
#define MS_RTC_NACK_GENERATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/SeqManager.hpp"
#include "handles/Timer.hpp"
#include <absl/container/btree_map.h>
#include <absl/container/btree_set.h>
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
			virtual ~Listener() = default;

		public:
			virtual void OnNackGeneratorNackRequired(const std::vector<uint16_t>& seqNumbers) = 0;
			virtual void OnNackGeneratorKeyFrameRequired()                                    = 0;
		};

	private:
		struct NackInfo
		{
			NackInfo() = default;
			explicit NackInfo(uint16_t seq, uint16_t sendAtSeq) : seq(seq), sendAtSeq(sendAtSeq)
			{
			}

			uint16_t seq{ 0u };
			uint16_t sendAtSeq{ 0u };
			uint64_t sentAtMs{ 0u };
			uint8_t retries{ 0u };
		};

		enum class NackFilter
		{
			SEQ,
			TIME
		};

	public:
		explicit NackGenerator(Listener* listener);
		~NackGenerator() override;

		bool ReceivePacket(RTC::RtpPacket* packet, bool isRecovered);
		size_t GetNackListLength() const
		{
			return this->nackList.size();
		}
		void UpdateRtt(uint32_t rtt)
		{
			this->rtt = rtt;
		}
		void Reset();

	private:
		void AddPacketsToNackList(uint16_t seqStart, uint16_t seqEnd);
		bool RemoveNackItemsUntilKeyFrame();
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
		absl::btree_map<uint16_t, NackInfo, RTC::SeqManager<uint16_t>::SeqLowerThan> nackList;
		absl::btree_set<uint16_t, RTC::SeqManager<uint16_t>::SeqLowerThan> keyFrameList;
		absl::btree_set<uint16_t, RTC::SeqManager<uint16_t>::SeqLowerThan> recoveredList;
		bool started{ false };
		uint16_t lastSeq{ 0u }; // Seq number of last valid packet.
		uint32_t rtt{ 0u };     // Round trip time (ms).
	};
} // namespace RTC

#endif
