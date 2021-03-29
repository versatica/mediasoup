#define MS_CLASS "DepLibWebRTC"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibWebRTC.hpp"
#include "Logger.hpp"
#include "system_wrappers/source/field_trial.h" // webrtc::field_trial
#include <mutex>

/* Static. */

static std::once_flag globalInitOnce;
static constexpr char FieldTrials[] = "WebRTC-Bwe-AlrLimitedBackoff/Enabled/";

/* Static methods. */

void DepLibWebRTC::ClassInit()
{
	MS_TRACE();

	std::call_once(globalInitOnce, [] { webrtc::field_trial::InitFieldTrialsFromString(FieldTrials); });
}

void DepLibWebRTC::ClassDestroy()
{
	MS_TRACE();
}
