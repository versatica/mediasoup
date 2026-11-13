#define MS_CLASS "RTC::KeyFrameRequestManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/KeyFrameRequestManager.hpp"
#include "Logger.hpp"

static constexpr int64_t KeyFrameRetransmissionWaitMs{ 1000 };

/* PendingKeyFrameInfo methods. */

RTC::PendingKeyFrameInfo::PendingKeyFrameInfo(
  PendingKeyFrameInfo::Listener* listener, SharedInterface* shared, uint32_t ssrc)
  : listener(listener),
    ssrc(ssrc),
    timer(shared->CreateTimer(this, "key-frame-request-manager-pending-key-frame-info"))
{
	MS_TRACE();

	this->timer->Start(KeyFrameRetransmissionWaitMs);
}

RTC::PendingKeyFrameInfo::~PendingKeyFrameInfo()
{
	MS_TRACE();
}

void RTC::PendingKeyFrameInfo::SetRetryOnTimeout(bool retryOnTimeout)
{
	MS_TRACE();

	this->retryOnTimeout = retryOnTimeout;
}

void RTC::PendingKeyFrameInfo::OnTimer(TimerHandleInterface* timer)
{
	MS_TRACE();

	if (timer == this->timer.get())
	{
		this->listener->OnKeyFrameRequestTimeout(this);
	}
}

/* KeyFrameRequestDelayer methods. */

RTC::KeyFrameRequestDelayer::KeyFrameRequestDelayer(
  KeyFrameRequestDelayer::Listener* listener, SharedInterface* shared, uint32_t ssrc, int64_t delayMs)
  : listener(listener),
    ssrc(ssrc),
    timer(shared->CreateTimer(this, "key-frame-request-manager-key-frame-request-delayer"))
{
	MS_TRACE();

	this->timer->Start(delayMs);
}

RTC::KeyFrameRequestDelayer::~KeyFrameRequestDelayer()
{
	MS_TRACE();
}

void RTC::KeyFrameRequestDelayer::SetKeyFrameRequested(bool keyFrameRequested)
{
	MS_TRACE();

	this->keyFrameRequested = keyFrameRequested;
}

void RTC::KeyFrameRequestDelayer::OnTimer(TimerHandleInterface* timer)
{
	MS_TRACE();

	if (timer == this->timer.get())
	{
		this->listener->OnKeyFrameDelayTimeout(this);
	}
}

/* KeyFrameRequestManager methods. */

RTC::KeyFrameRequestManager::KeyFrameRequestManager(
  KeyFrameRequestManager::Listener* listener, SharedInterface* shared, int64_t keyFrameRequestDelayMs)
  : listener(listener), shared(shared), keyFrameRequestDelayMs(keyFrameRequestDelayMs)
{
	MS_TRACE();
}

RTC::KeyFrameRequestManager::~KeyFrameRequestManager()
{
	MS_TRACE();
}

void RTC::KeyFrameRequestManager::KeyFrameNeeded(uint32_t ssrc)
{
	MS_TRACE();

	if (this->keyFrameRequestDelayMs > 0)
	{
		const auto it = this->mapSsrcKeyFrameRequestDelayer.find(ssrc);

		// There is a delayer for the given ssrc, so enable it and return.
		if (it != this->mapSsrcKeyFrameRequestDelayer.end())
		{
			MS_DEBUG_DEV("there is a delayer for the given ssrc, enabling it and returning");

			auto* const keyFrameRequestDelayer = it->second.get();

			keyFrameRequestDelayer->SetKeyFrameRequested(true);

			return;
		}
		// Otherwise create a delayer (not yet enabled) and continue.
		else
		{
			MS_DEBUG_DEV("creating a delayer for the given ssrc");

			this->mapSsrcKeyFrameRequestDelayer[ssrc] = std::make_unique<KeyFrameRequestDelayer>(
			  this, this->shared, ssrc, this->keyFrameRequestDelayMs);
		}
	}

	const auto it = this->mapSsrcPendingKeyFrameInfo.find(ssrc);

	// There is a pending key frame for the given ssrc.
	if (it != this->mapSsrcPendingKeyFrameInfo.end())
	{
		auto* const pendingKeyFrameInfo = it->second.get();

		// Re-request the key frame if not received on time.
		pendingKeyFrameInfo->SetRetryOnTimeout(true);

		return;
	}

	this->mapSsrcPendingKeyFrameInfo[ssrc] =
	  std::make_unique<PendingKeyFrameInfo>(this, this->shared, ssrc);

	this->listener->OnKeyFrameNeeded(this, ssrc);
}

void RTC::KeyFrameRequestManager::ForceKeyFrameNeeded(uint32_t ssrc)
{
	MS_TRACE();

	if (this->keyFrameRequestDelayMs > 0)
	{
		// Create/replace a delayer for this ssrc.
		// NOTE: The existing one must be deleted before creating the new one, since
		// both would hold a timer with the same label at once.
		this->mapSsrcKeyFrameRequestDelayer.erase(ssrc);
		this->mapSsrcKeyFrameRequestDelayer[ssrc] = std::make_unique<KeyFrameRequestDelayer>(
		  this, this->shared, ssrc, this->keyFrameRequestDelayMs);
	}

	const auto it = this->mapSsrcPendingKeyFrameInfo.find(ssrc);

	// There is a pending key frame for the given ssrc.
	if (it != this->mapSsrcPendingKeyFrameInfo.end())
	{
		auto* const pendingKeyFrameInfo = it->second.get();

		pendingKeyFrameInfo->SetRetryOnTimeout(true);
		pendingKeyFrameInfo->Restart();
	}
	else
	{
		this->mapSsrcPendingKeyFrameInfo[ssrc] =
		  std::make_unique<PendingKeyFrameInfo>(this, this->shared, ssrc);
	}

	this->listener->OnKeyFrameNeeded(this, ssrc);
}

void RTC::KeyFrameRequestManager::KeyFrameReceived(uint32_t ssrc)
{
	MS_TRACE();

	const auto it = this->mapSsrcPendingKeyFrameInfo.find(ssrc);

	// There is no pending key frame for the given ssrc.
	if (it == this->mapSsrcPendingKeyFrameInfo.end())
	{
		return;
	}

	this->mapSsrcPendingKeyFrameInfo.erase(it);
}

void RTC::KeyFrameRequestManager::OnKeyFrameRequestTimeout(PendingKeyFrameInfo* pendingKeyFrameInfo)
{
	MS_TRACE();

	const auto it = this->mapSsrcPendingKeyFrameInfo.find(pendingKeyFrameInfo->GetSsrc());

	MS_ASSERT(
	  it != this->mapSsrcPendingKeyFrameInfo.end(), "PendingKeyFrameInfo not present in the map");

	if (!pendingKeyFrameInfo->IsRetryOnTimeoutEnabled())
	{
		this->mapSsrcPendingKeyFrameInfo.erase(it);

		return;
	}

	// Best effort in case the PLI/FIR was lost. Do not retry on timeout.
	pendingKeyFrameInfo->SetRetryOnTimeout(false);
	pendingKeyFrameInfo->Restart();

	MS_DEBUG_DEV("requesting key frame on timeout");

	this->listener->OnKeyFrameNeeded(this, pendingKeyFrameInfo->GetSsrc());
}

void RTC::KeyFrameRequestManager::OnKeyFrameDelayTimeout(KeyFrameRequestDelayer* keyFrameRequestDelayer)
{
	MS_TRACE();

	const auto it = this->mapSsrcKeyFrameRequestDelayer.find(keyFrameRequestDelayer->GetSsrc());

	MS_ASSERT(
	  it != this->mapSsrcKeyFrameRequestDelayer.end(), "KeyFrameRequestDelayer not present in the map");

	const auto ssrc              = keyFrameRequestDelayer->GetSsrc();
	const auto keyFrameRequested = keyFrameRequestDelayer->IsKeyFrameRequested();

	this->mapSsrcKeyFrameRequestDelayer.erase(it);

	// Ask for a new key frame as normal if needed.
	if (keyFrameRequested)
	{
		MS_DEBUG_DEV("requesting key frame after delay timeout");

		KeyFrameNeeded(ssrc);
	}
}
