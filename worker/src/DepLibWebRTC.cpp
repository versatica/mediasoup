#define MS_CLASS "DepLibWebRTC"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibWebRTC.hpp"
#include "Logger.hpp"
#include "system_wrappers/source/field_trial.h" // webrtc::field_trial
#include <mutex>

/* Static. */

static std::once_flag globalInitOnce;
static constexpr char FieldTrials[] =
  "WebRTC-Bwe-AlrLimitedBackoff/Enabled/WebRTC-Bwe-LossBasedBweV2/Enabled:true,CandidateFactors:1.02|1.0|0.95,DelayBasedCandidate:true,HigherBwBiasFactor:0.0002,HigherLogBwBiasFactor:0.02,ObservationDurationLowerBound:250ms,InstantUpperBoundBwBalance:75kbps,BwRampupUpperBoundFactor:1000000.0,InstantUpperBoundTemporalWeightFactor:0.9,TemporalWeightFactor:0.9,MaxIncreaseFactor:1.3,NewtonStepSize:0.75,InherentLossUpperBoundBwBalance:75kbps,LossThresholdOfHighBandwidthPreference:0.15,NotIncreaseIfInherentLossLessThanAverageLoss:true,TrendlineIntegrationEnabled:true,TrendlineObservationsWindowSize:2,InstantUpperBoundLossOffset:0.085/";

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
