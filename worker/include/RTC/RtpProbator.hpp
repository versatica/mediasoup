#ifndef MS_RTC_RTP_PROBATOR_HPP
#define MS_RTC_RTP_PROBATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"

namespace RTC
{
	class RtpProbator : public Timer::Listener
	{
	public:
		static constexpr size_t ProbationRtpPacketSize{ 1000 };

	public:
		class Listener
		{
		public:
			virtual void OnRtpProbatorSendRtpPacket(RTC::RtpProbator* rtpProbator, RTC::RtpPacket* packet) = 0;
		};

	public:
		explicit RtpProbator(RTC::RtpProbator::Listener* listener, size_t probationPacketLen);
		virtual ~RtpProbator();

	public:
		void Start(uint32_t bitrate);
		void Stop();
		bool IsActive() const;

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
	}; // namespace RTC

	/* Inline methods. */

	inline bool RtpProbator::IsActive() const
	{
		return this->rtpPeriodicTimer->IsActive();
	}
} // namespace RTC

#endif
