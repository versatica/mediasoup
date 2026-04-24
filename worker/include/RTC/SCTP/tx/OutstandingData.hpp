#ifndef MS_RTC_SCTP_OUTSTANDING_DATA_HPP
#define MS_RTC_SCTP_OUTSTANDING_DATA_HPP

#include "common.hpp"
#include "RTC/SCTP/common/UnwrappedSequenceNumber.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <deque>
#include <limits>
#include <optional>
#include <ostream>
#include <set>
#include <span>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * This class keeps track of outstanding data Chunks (sent, not yet acked)
		 * and handles acking, nacking, rescheduling and abandoning.
		 *
		 * Items are added to this queue as they are sent and will be removed when
		 * the peer acks them using the cumulative TSN ack.
		 */
		class OutstandingData
		{
		public:
			static constexpr uint16_t MaxRetransmitsNoLimit{ std::numeric_limits<uint16_t>::max() };
			static constexpr uint16_t ExpiresAtMsInfinite{ 0 };

#ifdef MS_TEST
		public:
			/**
			 * State for DATA Chunks (message fragments) in the queue.
			 *
			 * @remarks
			 * - Used in tests.
			 */
			enum class State : uint8_t
			{
				/**
				 * The Chunk has been sent but not received yet (from the sender's point
				 * of view, as no SACK has been received yet that reference this Chunk).
				 */
				IN_FLIGHT,
				/**
				 * A SACK has been received which explicitly marked this Chunk as missing.
				 * It's now NACKED and may be retransmitted if NACKED enough times.
				 */
				NACKED,
				/**
				 * A Chunk that will be retransmitted when possible.
				 */
				TO_BE_RETRANSMITTED,
				/**
				 * A SACK has been received which explicitly marked this Chunk as
				 * received.
				 */
				ACKED,
				/**
				 * A Chunk whose message has expired or has been retransmitted too many
				 * times (RFC3758). It will not be retransmitted anymore.
				 */
				ABANDONED,
			};
#endif

		public:
			using UnwrappedTsn = UnwrappedSequenceNumber<uint32_t>;

		public:
			/**
			 * Contains variables scoped to a processing of an incoming SACK.
			 */
			struct AckInfo
			{
				explicit AckInfo(UnwrappedTsn cumulativeTsnAck) : highestTsnAcked(cumulativeTsnAck)
				{
				}

				/**
				 * Bytes acked by increasing `cumulativeTsnAck` and `gapAckBlocks`.
				 */
				size_t bytesAcked{ 0 };

				/**
				 * Indicates if this SACK indicates that packet loss has occurred. Just
				 * because a packet is missing in the SACK doesn't necessarily mean that
				 * there is packet loss as that packet might be in-flight and received
				 * out-of-order. But when it has been reported missing consecutive
				 * times, it will eventually be considered "lost" and this will be set.
				 */
				bool hasPacketLoss{ false };

				/**
				 * Highest TSN Newly Acknowledged, an SCTP variable.
				 */
				UnwrappedTsn highestTsnAcked;

				/**
				 * The set of lifecycle IDs that were acked using `cumulativeTsnAck`.
				 */
				std::vector<uint64_t> ackedLifecycleIds;

				/**
				 * The set of lifecycle IDs that were acked, but had been abandoned.
				 */
				std::vector<uint64_t> abandonedLifecycleIds;
			};

		private:
			/**
			 * A fragmented message's DATA Chunk while in the retransmission queue,
			 * and its associated metadata.
			 *
			 * @remarks
			 * - This data structure has been optimized for size, by ordering fields
			 *   to avoid unnecessary padding.
			 */
			class Item
			{
			public:
				enum class NackAction : uint8_t
				{
					NOTHING,
					RETRANSMIT,
					ABANDON,
				};

			private:
			private:
				enum class Lifecycle : uint8_t
				{
					/**
					 * The Chunk is alive (sent, received, etc).
					 */
					ACTIVE,
					/**
					 * The Chunk is scheduled to be retransmitted, and will then
					 * transition to become active.
					 */
					TO_BE_RETRANSMITTED,
					/**
					 * The Chunk has been abandoned. This is a terminal state.
					 */
					ABANDONED
				};

				enum class AckState : uint8_t
				{
					/**
					 * The Chunk is in-flight.
					 */
					UNACKED,
					/**
					 * The Chunk has been received and acknowledged.
					 */
					ACKED,
					/**
					 * The Chunk has been nacked and is possibly lost.
					 */
					NACKED
				};

			public:
				Item(
				  uint32_t messageId,
				  UserData data,
				  uint64_t timeSentMs,
				  uint16_t maxRetransmissions,
				  uint64_t expiresAtMs,
				  uint64_t lifecycleId);

				Item(const Item&) = delete;

				Item& operator=(const Item&) = delete;

			public:
				uint32_t GetMessageId() const
				{
					return this->messageId;
				}

				uint64_t GetTimeSentMs() const
				{
					return this->timeSentMs;
				}

				const UserData& GetData() const
				{
					return this->data;
				}

				/**
				 * Acks an item.
				 */
				void Ack();

				/**
				 * Nacks an item. If it has been nacked enough times, or if
				 * `retransmitNow` is set, it might be marked for retransmission. If
				 * the item has reached its max retransmission value, it will instead
				 * be abandoned. The action performed is indicated as return value.
				 */
				NackAction Nack(bool retransmitNow);

				/**
				 * Prepares the item to be retransmitted. Sets it as outstanding and
				 * clears all nack counters.
				 */
				void MarkAsRetransmitted();

				/**
				 * Marks this item as abandoned.
				 */
				void Abandon();

				bool IsOutstanding() const
				{
					return this->ackState != AckState::ACKED && this->lifecycle == Lifecycle::ACTIVE;
				}

				bool IsAcked() const
				{
					return this->ackState == AckState::ACKED;
				}

				bool IsNacked() const
				{
					return this->ackState == AckState::NACKED;
				}

				bool IsAbandoned() const
				{
					return this->lifecycle == Lifecycle::ABANDONED;
				}

				/**
				 * Indicates if this Chunk should be retransmitted.
				 */
				bool ShouldBeRetransmitted() const
				{
					return this->lifecycle == Lifecycle::TO_BE_RETRANSMITTED;
				}

				/**
				 * Indicates if this Chunk has ever been retransmitted.
				 */
				bool HasBeenRetransmitted() const
				{
					return this->numRetransmissions > 0;
				}

				/**
				 * Given the current time, and the current state of this DATA Chunk, it
				 * will indicate if it has expired (SCTP Partial Reliability Extension).
				 */
				bool HasExpired(uint64_t nowMs) const
				{
					return (
					  this->expiresAtMs != OutstandingData::ExpiresAtMsInfinite && this->expiresAtMs <= nowMs);
				}

				uint64_t GetLifecycleId() const
				{
					return this->lifecycleId;
				}

				const uint32_t messageId;
				// When the packet was sent, and placed in this queue.
				const uint64_t timeSentMs;
				// If the message was sent with a maximum number of retransmissions,
				// this is set to that number. The value zero (0) means that it will
				// never be retransmitted.
				const uint16_t maxRetransmissions;
				// Indicates the life cycle status of this Chunk.
				Lifecycle lifecycle{ Lifecycle::ACTIVE };
				// Indicates the presence of this Chunk, if it's in flight (UNACKED),
				// has been received (ACKED) or is possibly lost (NACKED).
				AckState ackState{ AckState::UNACKED };
				// The number of times the DATA Chunk has been nacked (by having
				// received a SACK which doesn't include it). Will be cleared on
				// retransmissions.
				uint8_t nackCount{ 0 };
				// The number of times the DATA Chunk has been retransmitted.
				uint16_t numRetransmissions{ 0 };
				// At this exact millisecond, the item is considered expired. If the
				// message is not to be expired, this is set to the infinite future.
				// NOTE: If 0 it means infinite time.
				const uint64_t expiresAtMs;
				// An optional lifecycle id, which may only be set for the last
				// fragment.
				const uint64_t lifecycleId;
				// The actual data to send/retransmit.
				const UserData data;
			};

		public:
			OutstandingData(
			  size_t dataChunkHeaderLength,
			  UnwrappedTsn lastCumulativeTsnAck,
			  std::function<bool(uint16_t /*streamId*/, uint32_t /*outgoingMessageId*/)> discardFromSendQueue);

		public:
			AckInfo HandleSack(
			  UnwrappedTsn cumulativeTsnAck,
			  std::span<const SackChunk::GapAckBlock> gapAckBlocks,
			  bool isInFastRecovery);

			/**
			 * Returns as many of the Chunks that are eligible for fast retransmissions
			 * and that would fit in a single packet of `maxLength`. The eligible
			 * Chunks that didn't fit will be marked for (normal) retransmission and
			 * will not be returned if this method is called again.
			 */
			std::vector<std::pair<uint32_t /*tsn*/, UserData>> GetChunksToBeFastRetransmitted(size_t maxLength);

			/**
			 * Given `maxLength` of space left in a packet, which Chunks can be added
			 * to it?
			 */
			std::vector<std::pair<uint32_t /*tsn*/, UserData>> GetChunksToBeRetransmitted(size_t maxLength);

			/**
			 * How many inflight bytes there are, as sent on the wire as packets.
			 */
			size_t GetUnackedPacketBytes() const
			{
				return this->unackedPacketBytes;
			}

			/**
			 * How many inflight bytes there are, counting only the payload.
			 */
			size_t GetUnackedPayloadBytes() const
			{
				return this->unackedPayloadBytes;
			}

			/**
			 * Returns the number of DATA Chunks that are in-flight (not acked or
			 * nacked).
			 */
			size_t GetUnackedItems() const
			{
				return this->unackedItems;
			}

			/**
			 * Given the current time `nowMs`, expire and abandon outstanding (sent
			 * at least once) Chunks that have a limited lifetime.
			 */
			void ExpireOutstandingChunks(uint64_t nowMs);

			bool IsEmpty() const
			{
				return this->outstandingData.empty();
			}

			bool HasDataToBeFastRetransmitted() const
			{
				return !this->toBeFastRetransmitted.empty();
			}

			bool HasDataToBeRetransmitted() const
			{
				return !this->toBeRetransmitted.empty() || !this->toBeFastRetransmitted.empty();
			}

			UnwrappedTsn GetLastCumulativeTsnAck() const
			{
				return this->lastCumulativeTsnAck;
			}

			UnwrappedTsn GetNextTsn() const
			{
				return this->GetHighestOutstandingTsn().GetNextValue();
			}

			UnwrappedTsn GetHighestOutstandingTsn() const;

			/**
			 * Schedules `data` to be sent, with the provided partial reliability
			 * parameters. Returns the TSN if the item was actually added and
			 * scheduled to be sent, and std::nullopt if it shouldn't be sent.
			 */
			std::optional<UnwrappedTsn> Insert(
			  uint32_t messageId,
			  const UserData& data,
			  uint64_t timeSentMs,
			  uint16_t maxRetransmissions = OutstandingData::MaxRetransmitsNoLimit,
			  uint64_t expiresAtMs        = OutstandingData::ExpiresAtMsInfinite,
			  uint64_t lifecycleId        = 0);

			/**
			 * Nacks all outstanding data.
			 */
			void NackAll();

			/**
			 * Creates a FORWARD-TSN Chunk and adds it to the given Packet.
			 */
			void CreateForwardTsn(Packet* packet) const;

			/**
			 * Creates an I-FORWARD-TSN Chunk and adds it to the given Packet.
			 */
			void CreateIForwardTsn(Packet* packet) const;

			/**
			 * Given the current time and a TSN, it returns the measured RTT between
			 * when the Chunk was sent and now. It takes into acccount Karn's
			 * algorithm, so if the Chunk has ever been retransmitted, it will return
			 * `std::nullopt`.
			 */
			std::optional<uint64_t> MeasureRtt(uint64_t nowMs, UnwrappedTsn tsn) const;

			/**
			 * Returns true if the next Chunk that is not acked by the peer has been
			 * abandoned, which means that a FORWARD-TSN should be sent.
			 */
			bool ShouldSendForwardTsn() const;

			/**
			 * Called when an outgoing stream reset is sent, marking the last assigned
			 * TSN as a breakpoint that a FORWARD-TSN shouldn't cross.
			 */
			void BeginResetStreams();

#ifdef MS_TEST
			/**
			 * Returns the internal state of all queued Chunks.
			 *
			 * @remarks
			 * - Used in tests.
			 */
			std::vector<std::pair<uint32_t /*tsn*/, State>> GetChunkStatesForTesting() const;
#endif

		private:
			/**
			 * Returns how large a Chunk will be, serialized, carrying the data.
			 */
			size_t GetSerializedChunkLength(const UserData& data) const;

			Item& GetItem(UnwrappedTsn tsn);

			const Item& GetItem(UnwrappedTsn tsn) const;

			/**
			 * Given a `cumulativeTsnAck` from an incoming SACK, will remove those
			 * items in the retransmission queue up until this value and will update
			 * `ackInfo` by setting `this->lastCumulativeTsnAck`.
			 */
			void RemoveAcked(UnwrappedTsn cumulativeTsnAck, AckInfo& ackInfo);

			/**
			 * Will mark the Chunks covered by the `gapAckBlocks` from an incoming
			 * SACK as "acked" and update `ackInfo` by adding new TSNs to
			 * `this->cumulativeTsnAck`.
			 */
			void AckGapBlocks(
			  UnwrappedTsn cumulativeTsnAck,
			  std::span<const SackChunk::GapAckBlock> gapAckBlocks,
			  AckInfo& ackInfo);

			/**
			 * Mark Chunks reported as "missing", as "nacked" or "to be retransmitted"
			 * depending how many times this has happened. Only packets up until
			 * `ackInfo.highestTsnAcked` (highest TSN newly acknowledged) are
			 * nacked/retransmitted. The method will set `ackInfo.hasPacketLoss`.
			 */
			void NackBetweenAckBlocks(
			  UnwrappedTsn cumulativeTsnAck,
			  std::span<const SackChunk::GapAckBlock> gapAckBlocks,
			  bool isInFastRecovery,
			  bool cumulativeTsnAckedAdvanced,
			  AckInfo& ackInfo);

			/**
			 * Process the acknowledgement of the Chunk referenced by `item` and
			 * updates state in `ackInfo` and the object's state.
			 */
			void AckChunk(AckInfo& ackInfo, UnwrappedTsn tsn, Item& item);

			/**
			 * Helper method to process an incoming nack of an item and perform the
			 * correct operations given the action indicated when nacking an item
			 * (e.g. retransmitting or abandoning). The return value indicate if an
			 * action was performed, meaning that packet loss was detected and acted
			 * upon. If `doFastRetransmit` is set and if the item has been nacked
			 * sufficiently many times so that it should be retransmitted, this will
			 * schedule it to be "fast retransmitted". This is only done just before
			 * going into fast recovery.
			 *
			 * @remarks
			 * - Note that since nacking an item may result in it becoming abandoned,
			 *   which in turn could alter `this->outstandingData`, any iterators are
			 *   invalidated after having called this method.
			 */
			bool NackItem(UnwrappedTsn tsn, bool retransmitNow, bool doFastRetransmit);

			/**
			 * Given that a message fragment, `item` has been abandoned, abandon all
			 * other fragments that share the same message - both never-before-sent
			 * fragments that are still in the SendQueue and outstanding Chunks.
			 */
			void AbandonAllFor(const OutstandingData::Item& item);

			std::vector<std::pair<uint32_t /*tsn*/, UserData>> ExtractChunksThatCanFit(
			  std::set<UnwrappedTsn>& chunks, size_t maxLength);

			void AssertIsConsistent() const;

		private:
			// The size of the data Chunk (DATA/I-DATA) header that is used.
			const size_t dataChunkHeaderLength;
			// The last cumulative TSN ack number.
			UnwrappedTsn lastCumulativeTsnAck;
			// Callback when to discard items from the send queue.
			std::function<bool(uint16_t /*streamId*/, uint32_t /*outgoingMessageId*/)> discardFromSendQueue;
			// Outstanding items. If non-empty, the first element has
			// `TSN=this->lastCumulativeTsnAck_ + 1` and the following items are in
			// strict increasing TSN order. The last item has
			// `TSN=GetHighestOutstandingTsn()`.
			std::deque<Item> outstandingData;
			// The number of bytes that are in-flight, counting only the payload.
			size_t unackedPayloadBytes{ 0 };
			// The number of bytes that are in-flight, as sent on the wire (as
			// packets).
			size_t unackedPacketBytes{ 0 };
			// The number of DATA Chunks that are in-flight (sent but not yet acked
			// or nacked).
			size_t unackedItems{ 0 };
			// Data Chunks that are eligible for fast retransmission.
			std::set<UnwrappedTsn> toBeFastRetransmitted;
			// Data Chunks that are to be retransmitted.
			std::set<UnwrappedTsn> toBeRetransmitted;
			// Wben a stream reset has begun, the "next TSN to assign" is added to
			// this set, and removed when the cum-ack TSN reaches it. This is used
			// to limit a FORWARD-TSN to reset streams past a "stream reset last
			// assigned TSN".
			// NOTE: dcsctp uses `webrtc::flat_set<UnwrappedTsn>` type which is more
			// efficient in read operations.
			std::set<UnwrappedTsn> streamResetBreakpointTsns;
		};

#ifdef MS_TEST
		/**
		 * For logging purposes in Catch2 tests.
		 */
		inline std::ostream& operator<<(std::ostream& os, OutstandingData::State state)
		{
			switch (state)
			{
				case OutstandingData::State::IN_FLIGHT:
				{
					return os << "IN_FLIGHT";
				}

				case OutstandingData::State::NACKED:
				{
					return os << "NACKED";
				}

				case OutstandingData::State::TO_BE_RETRANSMITTED:
				{
					return os << "TO_BE_RETRANSMITTED";
				}

				case OutstandingData::State::ACKED:
				{
					return os << "ACKED";
				}

				case OutstandingData::State::ABANDONED:
				{
					return os << "ABANDONED";
				}

				default:
				{
					return os << "UNKNOWN(" << static_cast<int>(state) << ")";
				}
			}
		}

		/**
		 * For Catch2 to print it nicely.
		 */
		inline std::ostream& operator<<(
		  std::ostream& os, const std::pair<uint32_t, OutstandingData::State>& s)
		{
			return os << "{tsn:" << s.first << ", state:" << s.second << "}";
		}
#endif
	} // namespace SCTP
} // namespace RTC

#endif
