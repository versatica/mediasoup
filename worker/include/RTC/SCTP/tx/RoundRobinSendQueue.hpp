#ifndef MS_RTC_SCTP_ROUND_ROBIN_SEND_QUEUE_HPP
#define MS_RTC_SCTP_ROUND_ROBIN_SEND_QUEUE_HPP

#include "common.hpp"
#include "RTC/SCTP/public/AssociationListenerInterface.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SCTP/tx/SendQueueInterface.hpp"
#include "RTC/SCTP/tx/StreamScheduler.hpp"
#include <deque>
#include <map>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The Round Robin send queue holds all messages that the client wants to
		 * send, but that haven't yet been split into chunks and fully sent on the
		 * wire.
		 *
		 * As defined in https://datatracker.ietf.org/doc/html/rfc8260#section-3.2,
		 * it will cycle to send messages from different streams. It will send all
		 * fragments from one message before continuing with a different message on
		 * possibly a different stream, until support for message interleaving has
		 * been implemented.
		 *
		 * As messages can be (requested to be) sent before the connection is
		 * properly established, this send queue is always present - even for closed
		 * connections.
		 *
		 * The send queue may trigger callbacks. `OnAssociationStreamBufferedAmountLow()`
		 * and `OnAssociationTotalBufferedAmountLow()` will be triggered as defined in
		 * their documentation. `OnAssociationLifecycleMessageExpired()` with
		 * `maybeDelivered=false` and `OnAssociationLifecycleMessageEnd()` will be
		 * triggered when messages have been expired, abandoned or discarded from the
		 * send queue. If a message is fully produced, meaning that the last fragment
		 * has been produced, the responsibility to send lifecycle events is then
		 * transferred to the retransmission queue, which is the one asking to
		 * produce the message.
		 */
		class RoundRobinSendQueue : public SendQueueInterface
		{
		private:
			struct MessageAttributes
			{
				bool isUnordered;
				uint16_t maxRetransmissions;
				uint64_t expiresAtMs;
				std::optional<uint64_t> lifecycleId;
			};

		private:
			/**
			 * Represents a value and a "low threshold" that when the value reaches or
			 * goes under the "low threshold", will trigger `onThresholdReached()`
			 * callback.
			 */
			class ThresholdWatcher
			{
			public:
				explicit ThresholdWatcher(std::function<void()> onThresholdReached)
				  : onThresholdReached(std::move(onThresholdReached))
				{
				}

				/**
				 * Increases the value.
				 */
				void Increase(size_t bytes)
				{
					this->value += bytes;
				}

				/**
				 * Decreases the value and triggers `onThresholdReached()` if it's at
				 * or below `this->lowThreshold`.
				 */
				void Decrease(size_t bytes);

				size_t GetValue() const
				{
					return this->value;
				}

				size_t GetLowThreshold() const
				{
					return this->lowThreshold;
				}

				void SetLowThreshold(size_t lowThreshold);

			private:
				const std::function<void()> onThresholdReached;
				size_t value{ 0 };
				size_t lowThreshold{ 0 };
			};

		private:
			/**
			 * Per-stream information.
			 */
			class OutgoingStream : public StreamScheduler::StreamProducer
			{
			public:
				OutgoingStream(
				  RoundRobinSendQueue* parent,
				  StreamScheduler* scheduler,
				  uint16_t streamId,
				  uint16_t priority,
				  std::function<void()> onBufferedAmountLow)
				  : parent(*parent),
				    schedulerStream(scheduler->CreateStream(this, streamId, priority)),
				    bufferedAmountThresholdWatcher(std::move(onBufferedAmountLow))
				{
				}

				uint16_t GetStreamId() const
				{
					return this->schedulerStream->GetStreamId();
				}

				/**
				 * Enqueues a message to this stream.
				 */
				void AddMessage(Message message, MessageAttributes attributes);

				// Implementing `StreamScheduler::StreamProducer`.

				std::optional<SendQueueInterface::DataToSend> Produce(uint64_t nowMs, size_t maxLength) override;

				size_t GetBytesToSendInNextMessage() const override;

				const ThresholdWatcher& GetBufferedAmount() const
				{
					return bufferedAmountThresholdWatcher;
				}

				ThresholdWatcher& GetBufferedAmount()
				{
					return bufferedAmountThresholdWatcher;
				}

				/**
				 * Discards a partially sent message, see `SendQueue::Discard()`.
				 */
				bool Discard(uint32_t outgoingMessageId);

				/**
				 * Pauses this stream, which is used before resetting it.
				 */
				void Pause();

				/**
				 * Resumes a paused stream.
				 */
				void Resume();

				bool IsReadyToBeReset() const
				{
					return this->pauseState == PauseState::PAUSED;
				}

				bool IsResetting() const
				{
					return this->pauseState == PauseState::RESETTING;
				}

				void SetAsResetting();

				/**
				 * Resets this stream, meaning MIDs and SSNs are set to zero.
				 */
				void Reset();

				/**
				 * Indicates if this stream has a partially sent message in it.
				 */
				bool HasPartiallySentMessage() const;

				uint16_t GetPriority() const
				{
					return this->schedulerStream->GetPriority();
				}

				void SetPriority(uint16_t priority)
				{
					this->schedulerStream->SetPriority(priority);
				}

			private:
				/**
				 * Streams are paused before they can be reset. To reset a stream, the
				 * socket sends an outgoing stream reset command with the TSN of the last
				 * fragment of the last message, so that receivers and senders can agree
				 * on when it stopped. And if the send queue is in the middle of sending
				 * a message, and without fragments not yet sent and without TSNs
				 * allocated to them, it will keep sending data until that message has
				 * ended.
				 */
				enum class PauseState : uint8_t
				{
					/**
					 * The stream is not paused, and not scheduled to be reset.
					 */
					NOT_PAUSED,

					/**
					 * The stream has requested to be reset/paused but is still producing
					 * fragments of a message that hasn't ended yet. When it does, it will
					 * transition to the `PAUSED` state.
					 */
					PENDING,

					/**
					 * The stream is fully paused and can be reset.
					 */
					PAUSED,

					/**
					 * The stream has been added to an outgoing stream reset request and a
					 * response from the peer hasn't been received yet.
					 */
					RESETTING,
				};

				// An enqueued message and metadata.
				struct Item
				{
					explicit Item(uint32_t outgoingMessageId, Message msg, MessageAttributes attributes)
					  : outgoingMessageId(outgoingMessageId),
					    message(std::move(msg)),
					    attributes(attributes),
					    remainingLength(message.GetPayloadLength())
					{
					}

					uint32_t outgoingMessageId;
					Message message;
					MessageAttributes attributes;
					// The remaining payload (offset and length) to be sent, when it has
					// been fragmented.
					size_t remainingOffset{ 0 };
					size_t remainingLength;
					// If set, an allocated Message ID and SSN. Will be allocated when the
					// first fragment is sent.
					std::optional<uint32_t> mid{ std::nullopt };
					std::optional<uint16_t> ssn{ std::nullopt };
					// The current Fragment Sequence Number, incremented for each fragment.
					uint32_t currentFsn{ 0 };
				};

				void HandleMessageExpired(OutgoingStream::Item& item);

				void AssertIsConsistent() const;

			private:
				RoundRobinSendQueue& parent;
				const std::unique_ptr<StreamScheduler::Stream> schedulerStream;
				// The current amount of buffered data.
				ThresholdWatcher bufferedAmountThresholdWatcher;
				PauseState pauseState = PauseState::NOT_PAUSED;
				// MIDs are different for unordered and ordered messages sent on a
				// stream.
				uint32_t nextUnorderedMid{ 0 };
				uint32_t nextOrderedMid{ 0 };
				uint16_t nextSsn{ 0 };
				// Enqueued messages, and metadata.
				std::deque<Item> items;
			};

		public:
			RoundRobinSendQueue(
			  AssociationListenerInterface& associationListener,
			  size_t mtu,
			  uint16_t defaultPriority,
			  size_t totalBufferedAmountLowThreshold);

			~RoundRobinSendQueue() override;

		public:
			/**
			 * Indicates if the buffer is empty.
			 */
			bool IsEmpty() const
			{
				return GetTotalBufferedAmount() == 0;
			}

			/**
			 * Adds the message to be sent using the `sendMessageOptions` provided.
			 * The current time should be in `nowMs`. Note that it's the responsibility
			 * of the caller to ensure that the buffer is not full (by calling
			 * `IsFull()`) before adding messages to it.
			 */
			void AddMessage(
			  uint64_t nowMs, Message message, const SendMessageOptions& sendMessageOptions = {});

			uint16_t GetStreamPriority(uint16_t streamId) const;

			void SetStreamPriority(uint16_t streamId, uint16_t priority);

			// Methods implementing `SendQueueInterface`.
		public:
			void EnableMessageInterleaving(bool enabled) override
			{
				this->scheduler.EnableMessageInterleaving(enabled);
			}

			std::optional<SendQueueInterface::DataToSend> Produce(uint64_t nowMs, size_t maxLength) override;

			bool Discard(uint16_t streamId, uint32_t outgoingMessageId) override;

			void PrepareResetStream(uint16_t streamId) override;

			bool HasStreamsReadyToBeReset() const override;

			std::vector<uint16_t> GetStreamsReadyToBeReset() override;

			void CommitResetStreams() override;

			void RollbackResetStreams() override;

			void Reset() override;

			size_t GetStreamBufferedAmount(uint16_t streamId) const override;

			size_t GetTotalBufferedAmount() const override
			{
				return this->totalBufferedAmountThresholdWatcher.GetValue();
			}

			size_t GetStreamBufferedAmountLowThreshold(uint16_t streamId) const override;

			void SetStreamBufferedAmountLowThreshold(uint16_t streamId, size_t bytes) override;

		private:
			OutgoingStream& GetOrCreateStreamInfo(uint16_t streamId);

			void AssertIsConsistent() const;

		private:
			AssociationListenerInterface& associationListener;
			const uint16_t defaultPriority;
			StreamScheduler scheduler;
			// The total amount of buffer data, for all streams.
			ThresholdWatcher totalBufferedAmountThresholdWatcher;
			uint32_t currentOutgoingMessageId{ 0 };
			// All streams, and messages added to those.
			std::map<uint16_t, OutgoingStream> streams;
		};
	} // namespace SCTP
} // namespace RTC

#endif
