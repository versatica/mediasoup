#define MS_CLASS "KeyFrameRequestManager"

#include "RTC/KeyFrameRequestManager.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

void RTC::KeyFrameRequestManager::KeyFrameNeeded(uint32_t ssrc)
{
	auto it = this->mapSsrcPendingKeyFrameInfo.find(ssrc);

	// There is a pending keyframe for the given ssrc.
	if (it != this->mapSsrcPendingKeyFrameInfo.end())
		return;

	this->mapSsrcPendingKeyFrameInfo[ssrc] = new PendingKeyFrameInfo(this, ssrc);

	this->listener->OnKeyFrameNeeded(ssrc);
}

void RTC::KeyFrameRequestManager::KeyFrameReceived(uint32_t ssrc)
{
	auto it = this->mapSsrcPendingKeyFrameInfo.find(ssrc);

	// There is no pending keyframe for the given ssrc.
	if (it == this->mapSsrcPendingKeyFrameInfo.end())
		return;

	auto* pendingKeyFrameInfo = it->second;
	delete pendingKeyFrameInfo;

	this->mapSsrcPendingKeyFrameInfo.erase(it);
}

void RTC::KeyFrameRequestManager::OnKeyFrameRequestTimeout(PendingKeyFrameInfo* pendingKeyFrameInfo)
{
	auto it = this->mapSsrcPendingKeyFrameInfo.find(pendingKeyFrameInfo->GetSsrc());

	MS_ASSERT(it != this->mapSsrcPendingKeyFrameInfo.end(), "PendingKeyFrameInfo not present in the map")

	delete pendingKeyFrameInfo;

	this->mapSsrcPendingKeyFrameInfo.erase(it);
}
