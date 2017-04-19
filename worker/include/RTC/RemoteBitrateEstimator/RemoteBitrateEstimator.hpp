/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// This class estimates the incoming available bandwidth.

#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include <map>
#include <vector>

namespace RTC
{
	// (jmillan) borrowed from webrtc/modules/include/module_common_types.h
	//
	// Interface used by the CallStats class to distribute call statistics.
	// Callbacks will be triggered as soon as the class has been registered to a
	// CallStats object using RegisterStatsObserver.
	class CallStatsObserver
	{
	public:
		virtual ~CallStatsObserver() = default;

		virtual void OnRttUpdate(int64_t avgRttMs, int64_t maxRttMs) = 0;
	};

	class RemoteBitrateEstimator : public CallStatsObserver
	{
	public:
		// Used to signal changes in bitrate estimates for the incoming streams.
		class Listener
		{
		public:
			// Called when a receive channel group has a new bitrate estimate for the incoming streams.
			virtual void OnReceiveBitrateChanged(const std::vector<uint32_t>& ssrcs, uint32_t bitrate) = 0;
		};

	protected:
		static const int64_t processIntervalMs{ 500 };
		static const int64_t streamTimeOutMs{ 2000 };

	public:
		~RemoteBitrateEstimator() override = default;

		// Called for each incoming packet. Updates the incoming payload bitrate
		// estimate and the over-use detector. If an over-use is detected the
		// remote bitrate estimate will be updated. Note that |payloadSize| is the
		// packet size excluding headers.
		// Note that |arrivalTimeMs| can be of an arbitrary time base.
		virtual void IncomingPacket(
		    int64_t arrivalTimeMs, size_t payloadSize, const RtpPacket& packet, uint32_t absSendTime) = 0;

		// Removes all data for |ssrc|.
		virtual void RemoveStream(uint32_t ssrc) = 0;
		// Returns true if a valid estimate exists and sets |bitrateBps| to the
		// estimated payload bitrate in bits per second. |ssrcs| is the list of ssrcs
		// currently being received and of which the bitrate estimate is based upon.
		virtual bool LatestEstimate(std::vector<uint32_t>* ssrcs, uint32_t* bitrateBps) const = 0;
		virtual void SetMinBitrate(int minBitrateBps)                                         = 0;
		// (jmillan) borrowed from webrtc/modules/include/module.h.
		//
		// Returns the number of milliseconds until the module wants a worker
		// thread to call Process.
		// This method is called on the same worker thread as Process will
		// be called on.
		// TODO(tommi): Almost all implementations of this function, need to know
		// the current tick count.  Consider passing it as an argument.  It could
		// also improve the accuracy of when the next callback occurs since the
		// thread that calls Process() will also have it's tick count reference
		// which might not match with what the implementations use.
		virtual int64_t TimeUntilNextProcess() = 0;
		// Process any pending tasks such as timeouts.
		// Called on a worker thread.
		virtual void Process() = 0;
	};
} // namespace RTC

#endif
