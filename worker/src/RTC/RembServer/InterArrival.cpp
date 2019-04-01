/*
 *  Copyright (c) 2013 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#define MS_CLASS "RTC::RembServer::InterArrival"
// #define MS_LOG_DEV

#include "RTC/RembServer/InterArrival.hpp"
#include "Logger.hpp"
#include "Utils.hpp" // LatestTimestamp
#include <algorithm>
#include <cmath> // std::lround()

namespace RTC
{
	namespace RembServer
	{
		static constexpr int BurstDeltaThresholdMs{ 5 };

		bool InterArrival::ComputeDeltas(
		  uint32_t timestamp,
		  int64_t arrivalTimeMs,
		  int64_t systemTimeMs,
		  size_t packetSize,
		  uint32_t* timestampDelta,
		  int64_t* arrivalTimeDeltaMs,
		  int* packetSizeDelta)
		{
			MS_TRACE();

			MS_ASSERT(timestampDelta, "'timestampDelta' missing");
			MS_ASSERT(arrivalTimeDeltaMs, "'arrivalTimeDeltaMs' missing");
			MS_ASSERT(packetSizeDelta, "'packetSizeDelta' missing");

			bool calculatedDeltas{ false };

			if (this->currentTimestampGroup.IsFirstPacket())
			{
				// We don't have enough data to update the filter, so we store it until we
				// have two frames of data to process.
				this->currentTimestampGroup.timestamp      = timestamp;
				this->currentTimestampGroup.firstTimestamp = timestamp;
			}
			else if (!PacketInOrder(timestamp))
			{
				return false;
			}
			else if (NewTimestampGroup(arrivalTimeMs, timestamp))
			{
				// First packet of a later frame, the previous frame sample is ready.
				if (this->prevTimestampGroup.completeTimeMs >= 0)
				{
					*timestampDelta =
					  this->currentTimestampGroup.timestamp - this->prevTimestampGroup.timestamp;
					*arrivalTimeDeltaMs =
					  this->currentTimestampGroup.completeTimeMs - this->prevTimestampGroup.completeTimeMs;

					// Check system time differences to see if we have an unproportional jump
					// in arrival time. In that case reset the inter-arrival computations.
					int64_t systemTimeDeltaMs =
					  this->currentTimestampGroup.lastSystemTimeMs - this->prevTimestampGroup.lastSystemTimeMs;

					if (*arrivalTimeDeltaMs - systemTimeDeltaMs >= ArrivalTimeOffsetThresholdMs)
					{
						MS_WARN_TAG(
						  rbe,
						  "the arrival time clock offset has changed, resetting"
						  "[diff:%" PRId64 "ms]",
						  *arrivalTimeDeltaMs - systemTimeDeltaMs);

						Reset();

						return false;
					}

					if (*arrivalTimeDeltaMs < 0)
					{
						// The group of packets has been reordered since receiving its local
						// arrival timestamp.
						++this->numConsecutiveReorderedPackets;
						if (this->numConsecutiveReorderedPackets >= ReorderedResetThreshold)
						{
							MS_WARN_TAG(
							  rbe,
							  "packets are being reordered on the path from the "
							  "socket to the bandwidth estimator, ignoring this "
							  "packet for bandwidth estimation, resetting");
							Reset();
						}

						return false;
					}

					this->numConsecutiveReorderedPackets = 0;

					MS_ASSERT(*arrivalTimeDeltaMs >= 0, "invalid arrivalTimeDeltaMs value");

					*packetSizeDelta = static_cast<int>(this->currentTimestampGroup.size) -
					                   static_cast<int>(this->prevTimestampGroup.size);
					calculatedDeltas = true;
				}

				this->prevTimestampGroup = this->currentTimestampGroup;
				// The new timestamp is now the current frame.
				this->currentTimestampGroup.firstTimestamp = timestamp;
				this->currentTimestampGroup.timestamp      = timestamp;
				this->currentTimestampGroup.size           = 0;
			}
			else
			{
				this->currentTimestampGroup.timestamp =
				  Utils::Time::LatestTimestamp(this->currentTimestampGroup.timestamp, timestamp);
			}

			// Accumulate the frame size.
			this->currentTimestampGroup.size += packetSize;
			this->currentTimestampGroup.completeTimeMs   = arrivalTimeMs;
			this->currentTimestampGroup.lastSystemTimeMs = systemTimeMs;

			return calculatedDeltas;
		}

		bool InterArrival::PacketInOrder(uint32_t timestamp)
		{
			MS_TRACE();

			if (this->currentTimestampGroup.IsFirstPacket())
				return true;

			// Assume that a diff which is bigger than half the timestamp interval
			// (32 bits) must be due to reordering. This code is almost identical to
			// that in IsNewerTimestamp() in module_common_types.h.
			uint32_t timestampDiff = timestamp - this->currentTimestampGroup.firstTimestamp;

			return timestampDiff < 0x80000000;
		}

		// Assumes that |timestamp| is not reordered compared to
		// |this->currentTimestampGroup|.
		bool InterArrival::NewTimestampGroup(int64_t arrivalTimeMs, uint32_t timestamp) const
		{
			MS_TRACE();

			if (this->currentTimestampGroup.IsFirstPacket())
				return false;

			if (BelongsToBurst(arrivalTimeMs, timestamp))
				return false;

			uint32_t timestampDiff = timestamp - this->currentTimestampGroup.firstTimestamp;

			return timestampDiff > this->timestampGroupLengthTicks;
		}

		bool InterArrival::BelongsToBurst(int64_t arrivalTimeMs, uint32_t timestamp) const
		{
			MS_TRACE();

			if (!this->burstGrouping)
				return false;

			MS_ASSERT(this->currentTimestampGroup.completeTimeMs >= 0, "invalid completeTimeMs value");

			int64_t arrivalTimeDeltaMs = arrivalTimeMs - this->currentTimestampGroup.completeTimeMs;
			uint32_t timestampDiff     = timestamp - this->currentTimestampGroup.timestamp;
			auto tsDeltaMs =
			  static_cast<int64_t>(std::lround(this->timestampToMsCoeff * timestampDiff + 0.5));

			if (tsDeltaMs == 0)
				return true;

			int propagationDeltaMs = arrivalTimeDeltaMs - tsDeltaMs;

			return propagationDeltaMs < 0 && arrivalTimeDeltaMs <= BurstDeltaThresholdMs;
		}

		void InterArrival::Reset()
		{
			MS_TRACE();

			this->numConsecutiveReorderedPackets = 0;
			this->currentTimestampGroup          = TimestampGroup();
			this->prevTimestampGroup             = TimestampGroup();
		}
	} // namespace RembServer
} // namespace RTC
