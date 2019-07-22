#ifndef MS_RTC_RTP_PROBATOR_HPP
#define MS_RTC_RTP_PROBATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"

namespace RTC
{
	// Max RTP length.
	constexpr uint32_t RtpProbatorSsrc{ 1234u };

	class RtpProbator : public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnRtpProbatorSendRtpPacket(RTC::RtpProbator* rtpProbator, RTC::RtpPacket* packet) = 0;
			virtual void OnRtpProbatorStep(RTC::RtpProbator* rtpProbator)  = 0;
			virtual void OnRtpProbatorEnded(RTC::RtpProbator* rtpProbator) = 0;
		};

	public:
		explicit RtpProbator(RTC::RtpProbator::Listener* listener, size_t probationPacketLen);
		virtual ~RtpProbator();

	public:
		void Start(uint32_t bitrate);
		void Stop();
		bool IsActive() const;

	private:
		void ReloadProbation();

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		// Given as argument.
		RTC::RtpProbator::Listener* listener{ nullptr };
		size_t probationPacketLen{ 0 };
		// Allocated by this.
		uint8_t* probationPacketBuffer{ nullptr };
		RTC::RtpPacket* probationPacket{ nullptr };
		Timer* rtpPeriodicTimer{ nullptr };
		// Others.
		uint32_t targetBitrate{ 0u };
		uint16_t numSteps{ 0u };
		uint16_t currentStep{ 0u };
		uint64_t stepStartedAt{ 0u };
		uint64_t targetRtpInterval{ 0u };
	}; // namespace RTC

	/* Inline methods. */

	inline bool RtpProbator::IsActive() const
	{
		return this->rtpPeriodicTimer->IsActive();
	}
} // namespace RTC

#endif
