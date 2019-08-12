#ifndef MS_RTC_RTP_PROBATOR_HPP
#define MS_RTC_RTP_PROBATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"

namespace RTC
{
	// SSRC of the probation RTP stream.
	constexpr uint32_t RtpProbatorSsrc{ 1234u };

	class RtpProbator : public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnRtpProbatorSendRtpPacket(RTC::RtpProbator* rtpProbator, RTC::RtpPacket* packet) = 0;
			virtual void OnRtpProbatorStepDone(RTC::RtpProbator* rtpProbator, bool last) = 0;
		};

	public:
		explicit RtpProbator(RTC::RtpProbator::Listener* listener, size_t probationPacketLen);
		virtual ~RtpProbator();

	public:
		void Start(uint32_t bitrate);
		void Stop();
		bool IsRunning() const;
		uint32_t GetTargetBitrate() const;

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
		bool running{ false };
		uint32_t targetBitrate{ 0u };
		uint16_t numSteps{ 0u };
		uint16_t currentStep{ 0u };
		uint64_t stepStartedAt{ 0u };
		uint32_t stepNumPacket{ 0u };
		uint64_t targetRtpInterval{ 0u };
	}; // namespace RTC

	/* Inline methods. */

	inline bool RtpProbator::IsRunning() const
	{
		return this->running;
	}

	inline uint32_t RtpProbator::GetTargetBitrate() const
	{
		return this->targetBitrate;
	}
} // namespace RTC

#endif
