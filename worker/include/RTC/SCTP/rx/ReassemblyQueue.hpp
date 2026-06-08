#ifndef MS_RTC_SCTP_REASSEMBLY_QUEUE_HPP
#define MS_RTC_SCTP_REASSEMBLY_QUEUE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include "RTC/SCTP/rx/ReassemblyStreamsInterface.hpp"
#include <deque>
#include <set>
#include <span>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Contains the received DATA / I-DATA chunks that haven't yet been
		 * reassembled, and reassembles chunks when possible.
		 *
		 * The actual assembly is handled by an implementation of the
		 * `ReassemblyStreamsInterface` interface.
		 *
		 * Except for reassembling fragmented messages, this class will also handle
		 * two less common operations. To handle the receiver-side of partial
		 * reliability (limited number of retransmissions or limited message
		 * lifetime) as well as stream resetting, which is used when a sender wishes
		 * to close SCTP streams.
		 *
		 * Partial reliability is handled when a FORWARD-TSN or I-FORWARD-TSN chunk
		 * is received, and it will simply delete any chunks matching the parameters
		 * in that chunk. This is mainly implemented in ReassemblyStreams classes.
		 *
		 * Resetting streams is handled when a RECONFIG chunks is received with an
		 * "Outgoing SSN Reset Request" parameter. That parameter will contain a list
		 * of streams to reset, and a `senderLastAssignedTsn`. If this TSN is not yet
		 * seen, the stream cannot be directly reset, and this class will respond
		 * that the reset is "deferred". But if this TSN provided is known, the
		 * stream can be immediately be reset.
		 *
		 * The reassembly queue has a maximum size, as it would otherwise be an DoS
		 * attack vector where a peer could consume all memory of the other peer by
		 * sending a lot of ordered chunks, but carefully withholding an early one.
		 * It also has a watermark limit, which the caller can query is the number
		 * of bytes is above that limit. This is used by the caller to be selective
		 * in what to add to the reassembly queue, so that it's not exhausted. The
		 * caller is expected to call `IsFull()` prior to adding data to the queue
		 * and to act accordingly if the queue is full.
		 */
		class ReassemblyQueue
		{
		public:
			/**
			 * When the queue is filled over this fraction (of its maximum size), the
			 * SCTP association should restrict incoming data to avoid filling up the
			 * queue.
			 */
			static constexpr float HighWatermarkLimit{ 0.9 };

		private:
			struct DeferredResetStreams
			{
				DeferredResetStreams(Types::UnwrappedTsn senderLastAssignedTsn, std::set<uint16_t> streamIds)
				  : senderLastAssignedTsn(senderLastAssignedTsn), streamIds(std::move(streamIds))
				{
				}

				Types::UnwrappedTsn senderLastAssignedTsn;
				std::set<uint16_t> streamIds;
				std::vector<std::function<void()>> deferredActions;
				// TODO: SCTP: Once we upgrade to C++23, replace with:
				// std::vector<std::move_only_function<void()>> deferredActions;
			};

		public:
			explicit ReassemblyQueue(size_t maxLengthBytes, bool useMessageInterleaving = false);

			~ReassemblyQueue();

		public:
			/**
			 * Adds a data chunk to the queue, with a `tsn` and other parameters in
			 * `data`.
			 */
			void AddData(uint32_t tsn, UserData data);

			/**
			 * Indicates if the reassembly queue has any reassembled messages that can
			 * be retrieved by calling `GetNextMessage()`.
			 */
			bool HasMessages() const
			{
				return !this->reassembledMessages.empty();
			}

			/**
			 * Returns the number of reassembled messages that are ready to be
			 * retrieved by calling `GetNextMessage()`.
			 */
			size_t GetMessagesReadyCount() const
			{
				return this->reassembledMessages.size();
			}

			/**
			 * Returns the next reassembled message or `std::nullopt` if there are no
			 * messages ready.
			 */
			std::optional<Message> GetNextMessage();

			/**
			 * Handles a FORWARD-TSN/I-FORWARD-TSN chunk, when the sender has indicated
			 * that the received (this class) should forget about some chunks. This is
			 * used to implement partial reliability.
			 */
			void HandleForwardTsn(
			  uint32_t newCumulativeTsn, std::span<const AnyForwardTsnChunk::SkippedStream> skippedStreams);

			/**
			 * Resets the provided streams and leaves deferred reset processing, if
			 * enabled.
			 */
			void ResetStreamsAndLeaveDeferredReset(std::span<const uint16_t> streamIds);

			/**
			 * Enters deferred reset processing.
			 */
			void EnterDeferredReset(uint32_t senderLastAssignedTsn, std::span<const uint16_t> streamIds);

			/**
			 * The number of payload bytes that have been queued. Note that the actual
			 * memory usage is higher due to additional overhead of tracking received
			 * data.
			 */
			size_t GetQueuedBytes() const
			{
				return this->queuedBytes;
			}

			/**
			 * The remaining bytes until the queue has reached the watermark limit.
			 */
			size_t GetRemainingBytes() const
			{
				return this->watermarkBytes - this->queuedBytes;
			}

			/**
			 * Indicates if the queue is full. Data should not be added to the queue
			 * when it's full.
			 */
			bool IsFull() const
			{
				return this->queuedBytes >= this->maxLengthBytes;
			}

			/**
			 * Indicates if the queue is above the watermark limit, which is a certain
			 * percentage of its size.
			 */
			bool IsAboveWatermark() const
			{
				return this->queuedBytes >= this->watermarkBytes;
			}

			/**
			 * Returns the watermark limit, in bytes.
			 */
			size_t GetWatermarkBytes() const
			{
				return this->watermarkBytes;
			}

		private:
			std::unique_ptr<ReassemblyStreamsInterface> CreateReassemblyStreams(
			  ReassemblyStreamsInterface::OnAssembledMessage onAssembledMessage,
			  bool useMessageInterleaving);

			void AddReassembledMessage(std::span<const Types::UnwrappedTsn> tsns, Message message);

			void AssertIsConsistent() const;

		private:
			const size_t maxLengthBytes;
			const size_t watermarkBytes;
			Types::UnwrappedTsn::Unwrapper tsnUnwrapper;
			// Messages that have been reassembled, and will be consumed from by
			// `GetNextMessage()`.
			std::deque<Message> reassembledMessages;
			// If present, "deferred reset processing" mode is active.
			std::optional<DeferredResetStreams> deferredResetStreams;
			// The number of "payload bytes" that are in this queue, in total.
			size_t queuedBytes = 0;
			// The actual implementation of ReassemblyStreams.
			std::unique_ptr<ReassemblyStreamsInterface> reassemblyStreams;
		};
	} // namespace SCTP
} // namespace RTC

#endif
