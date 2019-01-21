#ifndef MS_KEY_FRAME_REQUEST_MANAGER_HPP
#define MS_KEY_FRAME_REQUEST_MANAGER_HPP

#include "handles/Timer.hpp"
#include <map>

static uint16_t KeyFrameWaitTime{ 2000 };

namespace RTC
{
	class PendingKeyFrameInfo : public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnKeyFrameRequestTimeout(PendingKeyFrameInfo* keyFrameRequestInfo) = 0;
		};

	public:
		PendingKeyFrameInfo(Listener* listener, uint32_t ssrc);
		~PendingKeyFrameInfo();

		uint32_t GetSsrc() const;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		Listener* listener{ nullptr };
		uint32_t ssrc{ 0 };
		Timer* timer{ nullptr };
	};

	class KeyFrameRequestManager : public PendingKeyFrameInfo::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnKeyFrameNeeded(uint32_t ssrc) = 0;
		};

	public:
		explicit KeyFrameRequestManager(Listener* listener);
		~KeyFrameRequestManager() = default;

		void KeyFrameNeeded(uint32_t ssrc);
		void KeyFrameReceived(uint32_t ssrc);

		/* Pure virtual methods inherited from PendingKeyFrameInfo::Listener. */
	public:
		void OnKeyFrameRequestTimeout(PendingKeyFrameInfo* keyFrameRequestInfo) override;

	private:
		Listener* listener{ nullptr };
		std::map<uint32_t, PendingKeyFrameInfo*> mapSsrcPendingKeyFrameInfo;
	};
} // namespace RTC

/* Inline PendingKeyFrameInfo methods */

inline RTC::PendingKeyFrameInfo::PendingKeyFrameInfo(
  PendingKeyFrameInfo::Listener* listener, uint32_t ssrc)
  : listener(listener), ssrc(ssrc)
{
	this->timer = new Timer(this);
	this->timer->Start(KeyFrameWaitTime);
}

inline RTC::PendingKeyFrameInfo::~PendingKeyFrameInfo()
{
	this->timer->Stop();
	delete this->timer;
}

inline uint32_t RTC::PendingKeyFrameInfo::GetSsrc() const
{
	return this->ssrc;
}

inline void RTC::PendingKeyFrameInfo::OnTimer(Timer* timer)
{
	if (timer == this->timer)
	{
		this->listener->OnKeyFrameRequestTimeout(this);
	}
}

/* Inline KeyFrameRequestManager methods */

inline RTC::KeyFrameRequestManager::KeyFrameRequestManager(KeyFrameRequestManager::Listener* listener)
  : listener(listener)
{
}

#endif
