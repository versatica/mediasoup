#ifndef MS_RTC_SCTP_STREAM_SCHEDULER_HPP
#define MS_RTC_SCTP_STREAM_SCHEDULER_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/tx/SendQueueInterface.hpp"
#include <set>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * A parameterized stream scheduler. Currently, it implements the round robin
		 *  scheduling algorithm using virtual finish time. It is to be used as a
		 *  part of a send queue and will track all active streams (streams that have
		 *  any data that can be sent).
		 *
		 * The stream scheduler works with the concept of associating active streams
		 * with a "virtual finish time", which is the time when a stream is allowed
		 * to produce data. Streams are ordered by their virtual finish time, and
		 * the "current virtual time" will advance to the next following virtual
		 * finish time whenever a chunk is to be produced.
		 *
		 * In the round robin scheduling algorithm, a stream's virtual finish time
		 * will just increment by one (1) after having produced a chunk, which
		 * results in a round-robin scheduling.
		 *
		 * In WFQ scheduling algorithm, a stream's virtual finish time will be
		 * defined as the number of bytes in the next fragment to be sent, multiplied
		 * by theinverse of the stream's priority, meaning that a high priority - or
		 * a smaller fragment - results in a closer virtual finish time, compared to
		 * a stream with either a lower priority or a larger fragment to be sent.
		 *
		 * @remarks
		 * - When message interleaving is enabled, the WFQ (Weighted Fair Queueing)
		 * 	 scheduling algorithm will be used. And when it's not, round-robin
		 * 	 scheduling will be used instead.
		 */
		class StreamScheduler
		{
		public:
			class StreamProducer
			{
			public:
				virtual ~StreamProducer() = default;

			public:
				/**
				 * Produces a fragment of data to send. The current wall time is specified
				 * as `nowMs` and should be used to skip chunks with expired limited
				 * lifetime. The parameter `maxLength` specifies the maximum amount of
				 * actual payload that may be returned. If these constraints prevents the
				 * stream from sending some data, `std::nullopt` should be returned.
				 */
				virtual std::optional<SendQueueInterface::DataToSend> Produce(
				  uint64_t nowMs, size_t maxLength) = 0;

				/**
				 * Returns the number of payload bytes that is scheduled to be sent in the
				 * next enqueued message, or zero if there are no enqueued messages or if
				 * the stream has been actively paused.
				 */
				virtual size_t GetBytesToSendInNextMessage() const = 0;
			};

		public:
			class Stream
			{
			private:
				friend class StreamScheduler;

			private:
				Stream(StreamScheduler* parent, StreamProducer* producer, uint16_t streamId, uint16_t priority)
				  : parent(*parent),
				    producer(*producer),
				    streamId(streamId),
				    priority(priority),
				    inverseWeight(1.0 / std::max(static_cast<double>(priority), 1e-6))
				{
				}

			public:
				uint16_t GetStreamId() const
				{
					return this->streamId;
				}

				uint16_t GetPriority() const
				{
					return this->priority;
				}

				void SetPriority(uint16_t priority);

				/**
				 * Will activate the stream if it has any data to send. That is, if the
				 * callback to `GetBytesToSendInNextMessage()` returns non-zero. If the
				 * callback returns zero, the stream will not be made active.
				 */
				void MayMakeActive();

				/**
				 * Will remove the stream from the list of active streams, and will not
				 * try to produce data from it. To make it active again, call
				 * `MayMakeActive()`.
				 */
				void MakeInactive();

				/**
				 * Make the scheduler move to another message, or another stream. This
				 * is used to abort the scheduler from continuing producing fragments
				 * for the current message in case it's deleted.
				 */
				void ForceReschedule()
				{
					this->parent.ForceReschedule();
				}

			private:
				/**
				 * Produces a message from this stream. This will only be called on
				 * streams that have data.
				 */
				std::optional<SendQueueInterface::DataToSend> Produce(uint64_t nowMs, size_t maxLength);

				void MakeActive(size_t bytesToSendNext);

				void ForceMarkInactive();

				double GetCurrentTime() const
				{
					return this->currentVirtualTime;
				}

				double GetNextFinishTime() const
				{
					return this->nextFinishTime;
				}

				size_t GetBytesToSendInNextMessage() const
				{
					return this->producer.GetBytesToSendInNextMessage();
				}

				double CalculateFinishTime(size_t bytesToSendNext) const;

			private:
				StreamScheduler& parent;
				StreamProducer& producer;
				const uint16_t streamId;
				uint16_t priority;
				double inverseWeight;
				// This outgoing stream's "current" virtual time.
				double currentVirtualTime{ 0.0 };
				double nextFinishTime{ 0.0 };
			};

		private:
			struct ActiveStreamComparator
			{
				// Ordered by virtual finish time (primary), stream-id (secondary).
				bool operator()(Stream* a, Stream* b) const
				{
					const double aVft = a->GetNextFinishTime();
					const double bVft = b->GetNextFinishTime();

					if (aVft == bVft)
					{
						return a->GetStreamId() < b->GetStreamId();
					}

					return aVft < bVft;
				}
			};

		public:
			explicit StreamScheduler(size_t mtu)
			  : maxPayloadBytes(mtu - Packet::CommonHeaderLength - IDataChunk::IDataChunkHeaderLength)
			{
			}

		public:
			std::unique_ptr<Stream> CreateStream(StreamProducer* producer, uint16_t streamId, uint16_t priority)
			{
				return std::unique_ptr<Stream>(new Stream(this, producer, streamId, priority));
			}

			void EnableMessageInterleaving(bool enabled)
			{
				this->enableMessageInterleaving = enabled;
			}

			/**
			 * Makes the scheduler stop producing message from the current stream and
			 * re-evaluates which stream to produce from.
			 */
			void ForceReschedule()
			{
				this->currentlySendingAMessage = false;
			}

			/**
			 * Produces a fragment of data to send. The current wall time is specified
			 * as `nowMs` and will be used to skip chunks with expired limited
			 * lifetime. The parameter `maxLength` specifies the maximum amount of
			 * actual payload that may be returned. If no data can be produced,
			 * `std::nullopt` is returned.
			 */
			std::optional<SendQueueInterface::DataToSend> Produce(uint64_t nowMs, size_t maxLength);

			std::set<uint16_t> GetActiveStreamsForTesting() const;

		private:
			void AssertIsConsistent() const;

		private:
			const size_t maxPayloadBytes;
			// The current virtual time, as defined in the WFQ algorithm.
			double virtualTime{ 0.0 };
			// The current stream to send chunks from.
			Stream* currentStream{ nullptr };
			bool enableMessageInterleaving{ false };
			// Indicates if the streams is currently sending a message, and should
			// then (if message interleaving is not enabled) continue sending from
			// this stream until that message has been sent in full.
			bool currentlySendingAMessage{ false };
			// The currently active streams, ordered by virtual finish time.
			std::set<Stream*, ActiveStreamComparator> activeStreams;
		};
	} // namespace SCTP
} // namespace RTC

#endif
