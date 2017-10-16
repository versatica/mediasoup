/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef MS_RTC_REMOTE_BITRATE_ESTIMATOR_INTER_ARRIVAL_HPP
#define MS_RTC_REMOTE_BITRATE_ESTIMATOR_INTER_ARRIVAL_HPP

#include "common.hpp"

namespace RTC
{
	// Helper class to compute the inter-arrival time delta and the size delta
	// between two timestamp groups. A timestamp is a 32 bit unsigned number with
	// a client defined rate.
	class InterArrival
	{
	public:
		// After this many packet groups received out of order InterArrival will
		// reset, assuming that clocks have made a jump.
		static constexpr int ReorderedResetThreshold{ 3 };
		static constexpr int64_t ArrivalTimeOffsetThresholdMs{ 3000 };

		// A timestamp group is defined as all packets with a timestamp which are at
		// most timestampGroupLengthTicks older than the first timestamp in that
		// group.
		InterArrival(uint32_t timestampGroupLengthTicks, double timestampToMsCoeff, bool enableBurstGrouping);

		// This function returns true if a delta was computed, or false if the current
		// group is still incomplete or if only one group has been completed.
		// |timestamp| is the timestamp.
		// |arrivalTimeMs| is the local time at which the packet arrived.
		// |packetSize| is the size of the packet.
		// |timestampDelta| (output) is the computed timestamp delta.
		// |arrivalTimeDeltaMs| (output) is the computed arrival-time delta.
		// |packetSizeDelta| (output) is the computed size delta.
		bool ComputeDeltas(
		  uint32_t timestamp,
		  int64_t arrivalTimeMs,
		  int64_t systemTimeMs,
		  size_t packetSize,
		  uint32_t* timestampDelta,
		  int64_t* arrivalTimeDeltaMs,
		  int* packetSizeDelta);

	private:
		struct TimestampGroup
		{
			TimestampGroup();
			bool IsFirstPacket() const;

			size_t size{ 0 };
			uint32_t firstTimestamp{ 0 };
			uint32_t timestamp{ 0 };
			int64_t completeTimeMs{ 0 };
			int64_t lastSystemTimeMs{ 0 };
		};

		// Returns true if the packet with timestamp |timestamp| arrived in order.
		bool PacketInOrder(uint32_t timestamp);
		// Returns true if the last packet was the end of the current batch and the
		// packet with |timestamp| is the first of a new batch.
		bool NewTimestampGroup(int64_t arrivalTimeMs, uint32_t timestamp) const;
		bool BelongsToBurst(int64_t arrivalTimeMs, uint32_t timestamp) const;
		void Reset();

		const uint32_t timestampGroupLengthTicks;
		TimestampGroup currentTimestampGroup;
		TimestampGroup prevTimestampGroup;
		double timestampToMsCoeff{ 0 };
		bool burstGrouping{ false };
		int numConsecutiveReorderedPackets{ 0 };
	};

	/* Inline methods. */

	inline InterArrival::TimestampGroup::TimestampGroup()
	  : size(0), firstTimestamp(0), timestamp(0), completeTimeMs(-1)
	{
	}

	inline bool InterArrival::TimestampGroup::IsFirstPacket() const
	{
		return completeTimeMs == -1;
	}

	inline InterArrival::InterArrival(
	  uint32_t timestampGroupLengthTicks, double timestampToMsCoeff, bool enableBurstGrouping)
	  : timestampGroupLengthTicks(timestampGroupLengthTicks), currentTimestampGroup(),
	    prevTimestampGroup(), timestampToMsCoeff(timestampToMsCoeff),
	    burstGrouping(enableBurstGrouping), numConsecutiveReorderedPackets(0)
	{
	}
} // namespace RTC

#endif
