#ifndef MS_KEY_FRAME_REQUEST_MANAGER_HPP
#define MS_KEY_FRAME_REQUEST_MANAGER_HPP

#include "handles/Timer.hpp"
#include <map>

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
		void SetRetryOnTimeout(bool notify);
		bool GetRetryOnTimeout() const;
		void Restart();

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		Listener* listener{ nullptr };
		uint32_t ssrc{ 0 };
		Timer* timer{ nullptr };
		bool retryOnTimeout{ true };
	};

	class KeyFrameRequestManager : public PendingKeyFrameInfo::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnKeyFrameNeeded(KeyFrameRequestManager* keyFrameRequestManager, uint32_t ssrc) = 0;
		};

	public:
		explicit KeyFrameRequestManager(Listener* listener);
		virtual ~KeyFrameRequestManager();

		void KeyFrameNeeded(uint32_t ssrc);
		void ForceKeyFrameNeeded(uint32_t ssrc);
		void KeyFrameReceived(uint32_t ssrc);

		/* Pure virtual methods inherited from PendingKeyFrameInfo::Listener. */
	public:
		void OnKeyFrameRequestTimeout(PendingKeyFrameInfo* pendingKeyFrameInfo) override;

	private:
		Listener* listener{ nullptr };
		std::map<uint32_t, PendingKeyFrameInfo*> mapSsrcPendingKeyFrameInfo;
	};
} // namespace RTC

inline uint32_t RTC::PendingKeyFrameInfo::GetSsrc() const
{
	return this->ssrc;
}

inline void RTC::PendingKeyFrameInfo::SetRetryOnTimeout(bool notify)
{
	this->retryOnTimeout = notify;
}

inline bool RTC::PendingKeyFrameInfo::GetRetryOnTimeout() const
{
	return this->retryOnTimeout;
}

inline void RTC::PendingKeyFrameInfo::Restart()
{
	return this->timer->Restart();
}

#endif
