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

#ifndef MS_RTC_REMB_SERVER_REMOTE_BITRATE_ESTIMATOR_HPP
#define MS_RTC_REMB_SERVER_REMOTE_BITRATE_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include <map>
#include <vector>

namespace RTC
{
	namespace RembServer
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
				virtual void OnRembServerBitrateEstimation(
				  const RemoteBitrateEstimator* remoteBitrateEstimator,
				  const std::vector<uint32_t>& ssrcs,
				  uint32_t availableBitrate) = 0;
			};

		protected:
			static const int64_t streamTimeOutMs{ 2000 };

		public:
			~RemoteBitrateEstimator() override = default;

			// Called for each incoming packet. Updates the incoming payload bitrate
			// estimate and the over-use detector. If an over-use is detected the
			// remote bitrate estimate will be updated. Note that |payloadSize| is the
			// packet size excluding headers.
			// Note that |arrivalTimeMs| can be of an arbitrary time base.
			virtual void IncomingPacket(
			  int64_t arrivalTimeMs,
			  size_t payloadSize,
			  const RTC::RtpPacket& packet,
			  uint32_t absSendTime) = 0;

			// Removes all data for |ssrc|.
			virtual void RemoveStream(uint32_t ssrc) = 0;
			// Returns true if a valid estimate exists and sets |bitrateBps| to the
			// estimated payload bitrate in bits per second. |ssrcs| is the list of ssrcs
			// currently being received and of which the bitrate estimate is based upon.
			virtual bool LatestEstimate(std::vector<uint32_t>* ssrcs, uint32_t* bitrateBps) const = 0;
			virtual void SetMinBitrate(int minBitrateBps)                                         = 0;
			// Process any pending tasks such as timeouts.
			// Called on a worker thread.
			virtual void Process() = 0;

			uint32_t GetAvailableBitrate() const;

		protected:
			uint32_t availableBitrate{ 0 };
		};

		/* Inline methods. */

		inline uint32_t RemoteBitrateEstimator::GetAvailableBitrate() const
		{
			return this->availableBitrate;
		}
	} // namespace RembServer
} // namespace RTC

#endif
