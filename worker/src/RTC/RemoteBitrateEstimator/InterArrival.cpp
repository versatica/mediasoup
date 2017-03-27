/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "InterArrival"
// #define MS_LOG_DEV

#include "RTC/RemoteBitrateEstimator/InterArrival.hpp"
#include "Logger.hpp"
#include "Utils.hpp" // LatestTimestamp
#include <algorithm>

namespace RTC
{
	static const int kBurstDeltaThresholdMs = 5;

	bool InterArrival::ComputeDeltas(uint32_t timestamp, int64_t arrival_time_ms, int64_t system_time_ms, size_t packet_size, uint32_t* timestamp_delta,
	                                 int64_t* arrival_time_delta_ms, int* packet_size_delta)
	{
		MS_TRACE();

		MS_ASSERT(timestamp_delta, "'timestamp_delta' missing");
		MS_ASSERT(arrival_time_delta_ms, "'arrival_time_delta_ms' missing");
		MS_ASSERT(packet_size_delta, "'packet_size_delta' missing");

		bool calculated_deltas = false;

		if (this->currentTimestampGroup.IsFirstPacket())
		{
			// We don't have enough data to update the filter, so we store it until we
			// have two frames of data to process.
			this->currentTimestampGroup.timestamp = timestamp;
			this->currentTimestampGroup.first_timestamp = timestamp;
		}
		else if (!PacketInOrder(timestamp))
		{
			return false;
		}
		else if (NewTimestampGroup(arrival_time_ms, timestamp))
		{
			// First packet of a later frame, the previous frame sample is ready.
			if (this->prevTimestampGroup.complete_time_ms >= 0)
			{
				*timestamp_delta = this->currentTimestampGroup.timestamp - this->prevTimestampGroup.timestamp;
				*arrival_time_delta_ms = this->currentTimestampGroup.complete_time_ms - this->prevTimestampGroup.complete_time_ms;
				// Check system time differences to see if we have an unproportional jump
				// in arrival time. In that case reset the inter-arrival computations.
				int64_t system_time_delta_ms = this->currentTimestampGroup.last_system_time_ms - this->prevTimestampGroup.last_system_time_ms;
				if (*arrival_time_delta_ms - system_time_delta_ms >= kArrivalTimeOffsetThresholdMs)
				{
					MS_WARN_TAG(rbe, "the arrival time clock offset has changed, resetting [diff:%" PRId64 "ms]",
						*arrival_time_delta_ms - system_time_delta_ms);

					Reset();
					return false;
				}
				if (*arrival_time_delta_ms < 0)
				{
					// The group of packets has been reordered since receiving its local
					// arrival timestamp.
					++this->numConsecutiveReorderedPackets;
					if (this->numConsecutiveReorderedPackets >= kReorderedResetThreshold)
					{
						MS_WARN_TAG(rbe, "packets are being reordered on the path from the "
						                 "socket to the bandwidth estimator, ignoring this "
						                 "packet for bandwidth estimation, resetting");
						Reset();
					}
					return false;
				}
				else
				{
					this->numConsecutiveReorderedPackets = 0;
				}

				MS_ASSERT(*arrival_time_delta_ms >= 0, "invalid arrival_time_delta_ms value");

				*packet_size_delta = static_cast<int>(this->currentTimestampGroup.size) - static_cast<int>(this->prevTimestampGroup.size);
				calculated_deltas = true;
			}
			this->prevTimestampGroup = this->currentTimestampGroup;
			// The new timestamp is now the current frame.
			this->currentTimestampGroup.first_timestamp = timestamp;
			this->currentTimestampGroup.timestamp = timestamp;
			this->currentTimestampGroup.size = 0;
		}
		else
		{
			this->currentTimestampGroup.timestamp = Utils::Time::LatestTimestamp(this->currentTimestampGroup.timestamp, timestamp);
		}
		// Accumulate the frame size.
		this->currentTimestampGroup.size += packet_size;
		this->currentTimestampGroup.complete_time_ms = arrival_time_ms;
		this->currentTimestampGroup.last_system_time_ms = system_time_ms;

		return calculated_deltas;
	}

	bool InterArrival::PacketInOrder(uint32_t timestamp)
	{
		MS_TRACE();

		if (this->currentTimestampGroup.IsFirstPacket())
		{
			return true;
		}
		else
		{
			// Assume that a diff which is bigger than half the timestamp interval
			// (32 bits) must be due to reordering. This code is almost identical to
			// that in IsNewerTimestamp() in module_common_types.h.
			uint32_t timestamp_diff = timestamp - this->currentTimestampGroup.first_timestamp;

			return timestamp_diff < 0x80000000;
		}
	}

	// Assumes that |timestamp| is not reordered compared to
	// |this->currentTimestampGroup|.
	bool InterArrival::NewTimestampGroup(int64_t arrival_time_ms, uint32_t timestamp) const
	{
		MS_TRACE();

		if (this->currentTimestampGroup.IsFirstPacket())
		{
			return false;
		}
		else if (BelongsToBurst(arrival_time_ms, timestamp))
		{
			return false;
		}
		else
		{
			uint32_t timestamp_diff = timestamp - this->currentTimestampGroup.first_timestamp;
			return timestamp_diff > kTimestampGroupLengthTicks;
		}
	}

	bool InterArrival::BelongsToBurst(int64_t arrival_time_ms, uint32_t timestamp) const
	{
		MS_TRACE();

		if (!this->burstGrouping)
		{
			return false;
		}
		MS_ASSERT(this->currentTimestampGroup.complete_time_ms >= 0, "invalid complete_time_ms value");
		int64_t arrival_time_delta_ms = arrival_time_ms - this->currentTimestampGroup.complete_time_ms;
		uint32_t timestamp_diff = timestamp - this->currentTimestampGroup.timestamp;
		int64_t ts_delta_ms = this->timestampToMsCoeff * timestamp_diff + 0.5;

		if (ts_delta_ms == 0)
			return true;

		int propagation_delta_ms = arrival_time_delta_ms - ts_delta_ms;

		return propagation_delta_ms < 0 && arrival_time_delta_ms <= kBurstDeltaThresholdMs;
	}

	void InterArrival::Reset()
	{
		MS_TRACE();

		this->numConsecutiveReorderedPackets = 0;
		this->currentTimestampGroup = TimestampGroup();
		this->prevTimestampGroup = TimestampGroup();
	}
}
