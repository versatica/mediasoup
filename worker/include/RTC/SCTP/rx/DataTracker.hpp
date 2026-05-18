#ifndef MS_RTC_SCTP_DATA_TRACKER_HPP
#define MS_RTC_SCTP_DATA_TRACKER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "Utils/UnwrappedSequenceNumber.hpp"
#include "handles/BackoffTimerHandleInterface.hpp"
#include <set>
#include <string_view>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Keeps track of received DATA and I-DATA chunks and handles all logic for
		 * when_to create SACKs and also how_to generate them.
		 *
		 * It only uses TSNs to track delivery and doesn't need to be aware of
		 * streams.
		 *
		 * SACKs are optimally sent every second packet on association with no packet
		 * loss. When packet loss is detected, it's sent for every packet. When SACKs
		 * are not sent directly, a timer is used to send a SACK delayed (by RTO/2,
		 * or 200ms, whatever is smallest).
		 */
		class DataTracker
		{
		public:
			/**
			 * The maximum number of duplicate TSNs that will be reported in a SACK.
			 */
			static constexpr size_t MaxDuplicateTsnReported{ 20 };

			/**
			 * The maximum number of gap-ack-blocks that will be reported in a SACK.
			 */
			static constexpr size_t MaxGapAckBlocksReported{ 20 };

			/**
			 * The maximum number of accepted in-flight DATA / I-DATA chunks. This
			 * indicates the maximum difference from this buffer's last cumulative ack
			 * TSN, and any received data. Data received beyond this limit will be
			 * dropped, which will force the transmitter to send data that actually
			 * increases the last cumulative acked TSN.
			 */
			static constexpr uint32_t MaxAcceptedOutstandingFragments{ 100000 };

		private:
			enum class AckState : uint8_t
			{
				/**
				 * No need to send an ACK.
				 */
				IDLE,

				/**
				 * Has received data chunks (but not yet end of packet).
				 */
				BECOMING_DELAYED,

				/**
				 * Has received data chunks and the end of a packet. Delayed ack timer is
				 * running and a SACK will be sent on expiry, or if DATA / I-DATA is sent,
				 * or after next packet with data.
				 */
				DELAYED,

				/**
				 * Send a SACK immediately after handling this packet.
				 */
				IMMEDIATE,
			};

		private:
			static constexpr std::string_view AckStateToString(AckState ackState)
			{
				// NOTE: We cannot use MS_TRACE() here because clang in Linux will
				// complain about "read of non-constexpr variable 'configuration' is not
				// allowed in a constant expression".

				switch (ackState)
				{
					case AckState::IDLE:
					{
						return "IDLE";
					}

					case AckState::BECOMING_DELAYED:
					{
						return "BECOMING_DELAYED";
					}

					case AckState::DELAYED:
					{
						return "DELAYED";
					}

					case AckState::IMMEDIATE:
					{
						return "IMMEDIATE";
					}

						NO_DEFAULT_GCC();
				}
			}

		private:
			/**
			 * Represents ranges of TSNs that have been received that are not directly
			 * following the last cumulative acked TSN. This information is returned
			 * to the sender in the "gap-ack-blocks" in the SACK chunk. The blocks are
			 * always non-overlapping and non-adjacent.
			 */
			class AdditionalTsnBlocks
			{
			public:
				/**
				 * Represents an inclusive range of received TSNs, i.e. [firstTsn, lastTsn].
				 */
				struct TsnRange
				{
					TsnRange(Types::UnwrappedTsn firstTsn, Types::UnwrappedTsn lastTsn)
					  : firstTsn(firstTsn), lastTsn(lastTsn)
					{
					}

					Types::UnwrappedTsn firstTsn;
					Types::UnwrappedTsn lastTsn;
				};

				/**
				 * Adds a TSN to the set. This will try to expand any existing block and
				 * might merge blocks to ensure that all blocks are non-adjacent. If a
				 * current block can't be expanded, a new block is created.
				 *
				 * The return value indicates if `tsn` was added. If false is returned,
				 * the `tsn` was already represented in one of the blocks.
				 */
				bool Add(Types::UnwrappedTsn tsn);

				/**
				 * Erases all TSNs up to, and including `tsn`. This will remove all
				 * blocks that are completely below `tsn` and may truncate a block where
				 * `tsn` is within that block. In that case, the frontmost block's start
				 * TSN will be the next following tsn after `tsn`.
				 */
				void EraseTo(Types::UnwrappedTsn tsn);

				/**
				 * Removes the first block. Must not be called on an empty set.
				 */
				void PopFront();

				const std::vector<TsnRange>& GetBlocks() const
				{
					return this->blocks;
				}

				bool IsEmpty() const
				{
					return this->blocks.empty();
				}

				const TsnRange& Front() const
				{
					return this->blocks.front();
				}

			private:
				// A sorted vector of non-overlapping and non-adjacent blocks.
				std::vector<TsnRange> blocks;
			};

		public:
			DataTracker(BackoffTimerHandleInterface* delayedAckTimer, uint32_t remoteInitialTsn);

		public:
			/**
			 * Indicates if the provided TSN is valid. If this return false, the data
			 * should be dropped and not added to any other buffers, which essentially
			 * means that there is intentional packet loss.
			 */
			bool IsTsnValid(uint32_t tsn) const;

			/**
			 * Called for every incoming data chunk. Returns `true` if `tsn` was seen
			 * for the first time, and `false` if it has been seen before (a duplicate
			 * `tsn`).
			 *
			 * @remarks
			 * - `IsTsnValid()` must be called prior to calling this method.
			 */
			bool Observe(uint32_t tsn, bool immediateAck = false);

			/**
			 * Called at the end of processing an SCTP packet.
			 */
			void ObservePacketEnd();

			/**
			 * Called for incoming FORWARD-TSN/I-FORWARD-TSN chunks. Indicates if the
			 * chunk had any effect.
			 */
			bool HandleForwardTsn(uint32_t newCumulativeTsn);

			/**
			 * Indicates if a SACK should be sent. There may be other reasons to send
			 * a SACK, but if this function indicates so, it should be sent as soon as
			 * possible. Calling this function will make it clear a flag so that if
			 * it's called again, it will probably return `false`.
			 *
			 * If the delayed ack timer is running, this method will return `false`
			 * unless `alsoIfDelayed` is set to `true`. Then it will return true as
			 * well.
			 */
			bool ShouldSendAck(bool alsoIfDelayed = false);

			/**
			 * Forces `ShouldSendSack()` to return `true`.
			 */
			void ForceImmediateSack();

			/**
			 * Returns the last cumulative ack TSN - the last seen data chunk's TSN
			 * value before any packet loss was detected.
			 */
			uint32_t GetLastCumulativeAckedTsn() const
			{
				return this->lastCumulativeAckedTsn.Wrap();
			}

			bool IsLaterThanCumulativeAckedTsn(uint32_t tsn) const
			{
				return this->tsnUnwrapper.PeekUnwrap(tsn) > this->lastCumulativeAckedTsn;
			}

			/**
			 * Returns `true` if the received `tsn` would increase the cumulative ack
			 * TSN.
			 */
			bool WillIncreaseCumAckTsn(uint32_t tsn) const;

			/**
			 * Adds a SACK chunk with selective ack to the given Packet.
			 *
			 * @remarks
			 * - It will clear `this->duplicates`, so every SACK chunk that is
			 *   consumed must be sent.
			 */
			void AddSackSelectiveAck(Packet* packet, size_t aRwnd);

			void HandleDelayedAckTimerExpiry();

		private:
			void UpdateAckState(AckState newAckState, std::string_view reason);

		private:
			// If a packet has ever been seen.
			BackoffTimerHandleInterface* delayedAckTimer;
			bool packetSeen{ false };
			AckState ackState{ AckState::IDLE };
			Types::UnwrappedTsn::Unwrapper tsnUnwrapper;
			// All TSNs up until (and including) this value have been seen.
			Types::UnwrappedTsn lastCumulativeAckedTsn;
			// Received TSNs that are not directly following `lastCumulativeAckedTsn`.
			AdditionalTsnBlocks additionalTsnBlocks;
			std::set<uint32_t> duplicateTsns;
		};
	} // namespace SCTP
} // namespace RTC

#endif
