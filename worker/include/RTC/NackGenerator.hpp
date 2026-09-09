#ifndef MS_RTC_NACK_GENERATOR_HPP
#define MS_RTC_NACK_GENERATOR_HPP

#include "common.hpp"
#include "handles/TimerHandleInterface.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/SeqManager.hpp"
#include "SharedInterface.hpp"
#include <map>
#include <set>
#include <vector>

namespace RTC
{
	class NackGenerator : public TimerHandleInterface::Listener
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
			explicit NackInfo(int64_t createdAtMs, uint16_t seq, uint16_t sendAtSeq)
			  : createdAtMs(createdAtMs), seq(seq), sendAtSeq(sendAtSeq)
			{
			}

			int64_t createdAtMs{ 0 };
			uint16_t seq{ 0 };
			uint16_t sendAtSeq{ 0 };
			int64_t sentAtMs{ 0 };
			uint8_t retries{ 0 };
		};

		enum class NackFilter : uint8_t
		{
			SEQ,
			TIME
		};

	public:
		explicit NackGenerator(Listener* listener, SharedInterface* shared, int64_t sendNackDelayMs);
		~NackGenerator() override;

		bool ReceivePacket(const RTC::RTP::Packet* packet, bool isRecovered);
		size_t GetNackListLength() const
		{
			return this->nackList.size();
		}
		void UpdateRttMs(int64_t rttMs)
		{
			this->rttMs = rttMs;
		}
		void Reset();

	private:
		void AddPacketsToNackList(uint16_t seqStart, uint16_t seqEnd);
		bool RemoveNackItemsUntilKeyFrame();
		std::vector<uint16_t> GetNackBatch(NackFilter filter);
		void MayRunTimer() const;

		/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
	public:
		void OnTimer(TimerHandleInterface* timer) override;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		SharedInterface* shared{ nullptr };
		int64_t sendNackDelayMs{ 0 };
		// Allocated by this.
		TimerHandleInterface* timer{ nullptr };
		// Others.
		std::map<uint16_t, NackInfo, RTC::SeqManager<uint16_t>::SeqLowerThan> nackList;
		std::set<uint16_t, RTC::SeqManager<uint16_t>::SeqLowerThan> keyFrameList;
		std::set<uint16_t, RTC::SeqManager<uint16_t>::SeqLowerThan> recoveredList;
		bool started{ false };
		uint16_t lastSeq{ 0 }; // Seq number of last valid packet.
		int64_t rttMs{ 0 };
	};
} // namespace RTC

#endif
