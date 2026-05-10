#define MS_CLASS "RTC::SCTP::RoundRobinSendQueue"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/RoundRobinSendQueue.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <set>
#include <span>
#include <string>
#include <tuple> // std::forward_as_tuple()

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		RoundRobinSendQueue::RoundRobinSendQueue(
		  AssociationListener& associationListener,
		  size_t mtu,
		  uint16_t defaultPriority,
		  size_t totalBufferedAmountLowThreshold)
		  : associationListener(associationListener),
		    defaultPriority(defaultPriority),
		    scheduler(mtu),
		    totalBufferedAmountThresholdWatcher(
		      [this]()
		      {
			      this->associationListener.OnAssociationTotalBufferedAmountLow();
		      })
		{
			MS_TRACE();

			this->totalBufferedAmountThresholdWatcher.SetLowThreshold(totalBufferedAmountLowThreshold);
		}

		RoundRobinSendQueue::~RoundRobinSendQueue()
		{
			MS_TRACE();
		}

		void RoundRobinSendQueue::Add(
		  uint64_t nowMs, Message message, const SendMessageOptions& sendMessageOptions)
		{
			MS_TRACE();

			MS_ASSERT(message.GetPayloadLength(), "message payload cannot be empty");

			// Any limited lifetime should start counting from now - when the message
			// has been added to the queue.
			// `expiresAtMs` is the time when it expires. Which is slightly larger
			// than the message's lifetime, as the message is alive during its entire
			// lifetime (which may be zero).
			const MessageAttributes attributes = {
				.isUnordered        = sendMessageOptions.unordered,
				.maxRetransmissions = static_cast<uint16_t>(
				  sendMessageOptions.maxRetransmissions.has_value()
				    ? sendMessageOptions.maxRetransmissions.value()
				    : Types::MaxRetransmitsNoLimit),
				.expiresAtMs = sendMessageOptions.lifetimeMs.has_value()
				                 ? nowMs + sendMessageOptions.lifetimeMs.value() + 1
				                 : Types::ExpiresAtMsInfinite,
				.lifecycleId = sendMessageOptions.lifecycleId.value_or(0),
			};

			const uint16_t streamId = message.GetStreamId();

			GetOrCreateStreamInfo(streamId).Add(std::move(message), attributes);

			AssertIsConsistent();
		}

		uint16_t RoundRobinSendQueue::GetStreamPriority(uint16_t streamId) const
		{
			MS_TRACE();

			const auto it = this->streams.find(streamId);

			if (it == this->streams.end())
			{
				return this->defaultPriority;
			}

			return it->second.GetPriority();
		}

		void RoundRobinSendQueue::SetStreamPriority(uint16_t streamId, uint16_t priority)
		{
			MS_TRACE();

			OutgoingStream& stream = GetOrCreateStreamInfo(streamId);

			stream.SetPriority(priority);

			AssertIsConsistent();
		}

		std::optional<SendQueueInterface::DataToSend> RoundRobinSendQueue::Produce(
		  uint64_t nowMs, size_t maxLength)
		{
			MS_TRACE();

			return this->scheduler.Produce(nowMs, maxLength);
		}

		bool RoundRobinSendQueue::Discard(uint16_t streamId, uint32_t outgoingMessageId)
		{
			MS_TRACE();

			const bool hasDiscarded = GetOrCreateStreamInfo(streamId).Discard(outgoingMessageId);

			AssertIsConsistent();

			return hasDiscarded;
		}

		void RoundRobinSendQueue::PrepareResetStream(uint16_t streamId)
		{
			MS_TRACE();

			GetOrCreateStreamInfo(streamId).Pause();

			AssertIsConsistent();
		}

		bool RoundRobinSendQueue::HasStreamsReadyToBeReset() const
		{
			MS_TRACE();

			return std::ranges::any_of(
			  this->streams,
			  [](const auto& entry)
			  {
				  const auto& stream = entry.second;

				  return stream.IsReadyToBeReset();
			  });
		}

		std::vector<uint16_t> RoundRobinSendQueue::GetStreamsReadyToBeReset()
		{
			MS_TRACE();

			MS_ASSERT(
			  std::ranges::count_if(
			    this->streams,
			    [](const auto& s)
			    {
				    const auto& stream = s.second;

				    return stream.IsResetting();
			    }) == 0,
			  "none of the streams can be resetting");

			std::vector<uint16_t /*streamId*/> readyStreams;

			for (auto& [streamId, stream] : this->streams)
			{
				if (stream.IsReadyToBeReset())
				{
					stream.SetAsResetting();
					readyStreams.push_back(streamId);
				}
			}

			return readyStreams;
		}

		void RoundRobinSendQueue::CommitResetStreams()
		{
			MS_TRACE();

			MS_ASSERT(
			  std::ranges::count_if(
			    this->streams,
			    [](const auto& s)
			    {
				    const auto& stream = s.second;

				    return stream.IsResetting();
			    }) > 0,
			  "at least one stream must be resetting");

			for (auto& [unused, stream] : this->streams)
			{
				if (stream.IsResetting())
				{
					stream.Reset();
				}
			}

			AssertIsConsistent();
		}

		void RoundRobinSendQueue::RollbackResetStreams()
		{
			MS_TRACE();

			MS_ASSERT(
			  std::ranges::count_if(
			    this->streams,
			    [](const auto& s)
			    {
				    const auto& stream = s.second;

				    return stream.IsResetting();
			    }) > 0,
			  "at least one stream must be resetting");

			for (auto& [unused, stream] : this->streams)
			{
				if (stream.IsResetting())
				{
					stream.Resume();
				}
			}

			AssertIsConsistent();
		}

		void RoundRobinSendQueue::Reset()
		{
			MS_TRACE();

			// Recalculate buffered amount, as partially sent messages may have been
			// put fully back in the queue.
			for (auto& [unused, stream] : this->streams)
			{
				stream.Reset();
			}

			this->scheduler.ForceReschedule();
		}

		size_t RoundRobinSendQueue::GetStreamBufferedAmount(uint16_t streamId) const
		{
			MS_TRACE();

			const auto it = this->streams.find(streamId);

			if (it == this->streams.end())
			{
				return 0;
			}

			const auto& stream = it->second;

			return stream.GetBufferedAmount().GetValue();
		}

		size_t RoundRobinSendQueue::GetStreamBufferedAmountLowThreshold(uint16_t streamId) const
		{
			MS_TRACE();

			const auto it = this->streams.find(streamId);

			if (it == this->streams.end())
			{
				return 0;
			}

			const auto& stream = it->second;

			return stream.GetBufferedAmount().GetLowThreshold();
		}

		void RoundRobinSendQueue::SetStreamBufferedAmountLowThreshold(uint16_t streamId, size_t bytes)
		{
			MS_TRACE();

			GetOrCreateStreamInfo(streamId).GetBufferedAmount().SetLowThreshold(bytes);
		}

		RoundRobinSendQueue::OutgoingStream& RoundRobinSendQueue::GetOrCreateStreamInfo(uint16_t streamId)
		{
			MS_TRACE();

			const auto it = this->streams.find(streamId);

			if (it != this->streams.end())
			{
				auto& stream = it->second;

				return stream;
			}

			const auto [it2, inserted] = this->streams.emplace(
			  std::piecewise_construct,
			  std::forward_as_tuple(streamId),
			  std::forward_as_tuple(
			    this,
			    std::addressof(this->scheduler),
			    streamId,
			    this->defaultPriority,
			    [this, streamId]()
			    {
				    this->associationListener.OnAssociationStreamBufferedAmountLow(streamId);
			    }));

			return it2->second;
		}

		void RoundRobinSendQueue::AssertIsConsistent() const
		{
			MS_TRACE();

			std::set<uint16_t> expectedActiveStreamIds;
			const std::set<uint16_t> actualActiveStreamIds = this->scheduler.GetActiveStreamsForTesting();

			size_t totalBufferedAmount{ 0 };

			for (const auto& [streamId, stream] : this->streams)
			{
				totalBufferedAmount += stream.GetBufferedAmount().GetValue();

				if (stream.GetBytesToSendInNextMessage() > 0)
				{
					expectedActiveStreamIds.emplace(streamId);
				}
			}

			if (expectedActiveStreamIds != actualActiveStreamIds)
			{
				const auto join = [](const auto& set)
				{
					std::string out;

					size_t i{ 0 };
					for (const auto& v : set)
					{
						if (i++ != 0)
						{
							out += ",";
						}

						out += std::to_string(v);
					}

					return out;
				};

				MS_ASSERT(
				  expectedActiveStreamIds == actualActiveStreamIds,
				  "active streams mismatch [actual=[%s], expected=[%s]]",
				  join(actualActiveStreamIds).c_str(),
				  join(expectedActiveStreamIds).c_str());
			}

			MS_ASSERT(
			  totalBufferedAmount == this->totalBufferedAmountThresholdWatcher.GetValue(),
			  "total buffered amount mismatch [actual=[%zu], expected=[%zu]]",
			  totalBufferedAmount,
			  this->totalBufferedAmountThresholdWatcher.GetValue());
		}

		void RoundRobinSendQueue::ThresholdWatcher::Decrease(size_t bytes)
		{
			MS_TRACE();

			MS_ASSERT(
			  bytes <= this->value,
			  "bytes (%zu) must be smaller or equal than this->value (%zu)",
			  bytes,
			  this->value);

			const size_t oldValue = this->value;

			this->value -= bytes;

			if (oldValue > this->lowThreshold && this->value <= this->lowThreshold)
			{
				this->onThresholdReached();
			}
		}

		void RoundRobinSendQueue::ThresholdWatcher::SetLowThreshold(size_t lowThreshold)
		{
			MS_TRACE();

			// Betting on https://github.com/w3c/webrtc-pc/issues/2654 being accepted.
			if (this->lowThreshold < this->value && lowThreshold >= this->value)
			{
				this->onThresholdReached();
			}

			this->lowThreshold = lowThreshold;
		}

		void RoundRobinSendQueue::OutgoingStream::Add(Message message, MessageAttributes attributes)
		{
			MS_TRACE();

			const bool wasActive = GetBytesToSendInNextMessage() > 0;

			this->bufferedAmountThresholdWatcher.Increase(message.GetPayloadLength());
			this->parent.totalBufferedAmountThresholdWatcher.Increase(message.GetPayloadLength());

			const uint32_t outgoingMessageId = this->parent.currentOutgoingMessageId;

			this->parent.currentOutgoingMessageId = this->parent.currentOutgoingMessageId + 1;

			this->items.emplace_back(outgoingMessageId, std::move(message), attributes);

			if (!wasActive)
			{
				this->schedulerStream->MayMakeActive();
			}

			AssertIsConsistent();
		}

		std::optional<SendQueueInterface::DataToSend> RoundRobinSendQueue::OutgoingStream::Produce(
		  uint64_t nowMs, size_t maxLength)
		{
			MS_TRACE();

			MS_ASSERT(
			  this->pauseState != PauseState::PAUSED && this->pauseState != PauseState::RESETTING,
			  "pause state must not be PAUSED or RESETTING");

			while (!this->items.empty())
			{
				Item& item       = this->items.front();
				Message& message = item.message;

				// Allocate Message ID and SSN when the first fragment is sent.
				if (!item.mid.has_value())
				{
					// This entire message has already expired. Try the next one.
					if (item.attributes.expiresAtMs != Types::ExpiresAtMsInfinite && item.attributes.expiresAtMs <= nowMs)
					{
						HandleMessageExpired(item);

						this->items.pop_front();

						continue;
					}

					uint32_t& mid = item.attributes.isUnordered ? this->nextUnorderedMid : this->nextOrderedMid;

					item.mid = mid;
					mid += 1;
				}
				if (!item.attributes.isUnordered && !item.ssn.has_value())
				{
					item.ssn = this->nextSsn;

					this->nextSsn += 1;
				}

				// Grab the next `maxLength` fragment from this message and calculate
				// flags.
				const std::span<const uint8_t> messagePayload = message.GetPayload();
				const std::span<const uint8_t> chunkPayload   = messagePayload.subspan(
				  item.remainingOffset, std::min(messagePayload.size() - item.remainingOffset, maxLength));

				const bool isBeginning = chunkPayload.data() == messagePayload.data();
				const bool isEnd       = (chunkPayload.data() + chunkPayload.size()) ==
				                         (messagePayload.data() + messagePayload.size());

				const uint16_t streamId = message.GetStreamId();
				const uint32_t ppid     = message.GetPayloadProtocolId();

				// Zero-copy the payload if the message fits in a single chunk.
				std::vector<uint8_t> payload =
				  isBeginning && isEnd ? std::move(message).ReleasePayload()
				                       : std::vector<uint8_t>(chunkPayload.begin(), chunkPayload.end());

				const uint32_t fsn = item.currentFsn;

				item.currentFsn += 1;

				this->bufferedAmountThresholdWatcher.Decrease(payload.size());
				this->parent.totalBufferedAmountThresholdWatcher.Decrease(payload.size());

				SendQueueInterface::DataToSend dataToSend(
				  item.outgoingMessageId,
				  UserData(
				    streamId,
				    item.ssn.value_or(0),
				    *item.mid,
				    fsn,
				    ppid,
				    std::move(payload),
				    isBeginning,
				    isEnd,
				    item.attributes.isUnordered));

				dataToSend.maxRetransmissions = item.attributes.maxRetransmissions;
				dataToSend.expiresAtMs        = item.attributes.expiresAtMs;
				dataToSend.lifecycleId        = isEnd ? item.attributes.lifecycleId : 0;

				if (isEnd)
				{
					// The entire message has been sent, and its last data copied to
					// `dataToSend`, so it can safely be discarded.
					this->items.pop_front();

					if (this->pauseState == PauseState::PENDING)
					{
						MS_DEBUG_DEV(
						  "pause state on stream %" PRIu16 " is moving from PENDING to PAUSED", streamId);

						this->pauseState = PauseState::PAUSED;
					}
				}
				else
				{
					item.remainingOffset += chunkPayload.size();
					item.remainingLength -= chunkPayload.size();

					MS_ASSERT(
					  item.remainingOffset + item.remainingLength == item.message.GetPayloadLength(),
					  "item.remainingOffset + item.remainingLength != item.message.GetPayloadLength()");
					MS_ASSERT(item.remainingLength > 0, "item.remainingLength <= 0");
				}

				AssertIsConsistent();

				return dataToSend;
			}

			AssertIsConsistent();

			return std::nullopt;
		}

		size_t RoundRobinSendQueue::OutgoingStream::GetBytesToSendInNextMessage() const
		{
			MS_TRACE();

			if (this->pauseState == PauseState::PAUSED || this->pauseState == PauseState::RESETTING)
			{
				// The stream has paused (and there is no partially sent message).
				return 0;
			}

			if (this->items.empty())
			{
				return 0;
			}

			return this->items.front().remainingLength;
		}

		bool RoundRobinSendQueue::OutgoingStream::Discard(uint32_t outgoingMessageId)
		{
			MS_TRACE();

			bool result = false;

			if (!this->items.empty())
			{
				Item& item = this->items.front();

				if (item.outgoingMessageId == outgoingMessageId)
				{
					HandleMessageExpired(item);

					this->items.pop_front();

					// Only partially sent messages are discarded, so if a message was
					// discarded, then it was the currently sent message.
					this->schedulerStream->ForceReschedule();

					if (this->pauseState == PauseState::PENDING)
					{
						this->pauseState = PauseState::PAUSED;
						this->schedulerStream->MakeInactive();
					}
					else if (GetBytesToSendInNextMessage() == 0)
					{
						this->schedulerStream->MakeInactive();
					}

					// As the item still existed, it had unsent data.
					result = true;
				}
			}

			AssertIsConsistent();

			return result;
		}

		void RoundRobinSendQueue::OutgoingStream::Pause()
		{
			MS_TRACE();

			if (this->pauseState != PauseState::NOT_PAUSED)
			{
				// Already in progress.
				return;
			}

			const bool hadPendingItems = !this->items.empty();

			// https://datatracker.ietf.org/doc/html/rfc8831#section-6.7
			//
			// "Closing of a data channel MUST be signaled by resetting the corresponding
			// outgoing streams [RFC6525].  This means that if one side decides to close
			// the data channel, it resets the corresponding outgoing stream."
			// ... "[RFC6525] also guarantees that all the messages are delivered (or
			// abandoned) before the stream is reset."

			// A stream is paused when it's about to be reset. In this implementation,
			// it will throw away all non-partially send messages - they will be
			// abandoned as noted above. This is subject to change. It will however
			// not discard any partially sent messages - only whole messages. Partially
			// delivered messages (at the time of receiving a Stream Reset command) will
			// always deliver all the fragments before actually resetting the stream.
			for (auto it = this->items.begin(); it != this->items.end();)
			{
				if (it->remainingOffset == 0)
				{
					auto& item = *it;

					HandleMessageExpired(item);

					it = this->items.erase(it);
				}
				else
				{
					++it;
				}
			}

			this->pauseState = (this->items.empty() || this->items.front().remainingOffset == 0)
			                     ? PauseState::PAUSED
			                     : PauseState::PENDING;

			if (hadPendingItems && this->pauseState == PauseState::PAUSED)
			{
				MS_DEBUG_DEV("stream %" PRIu16 " was previously active, but is now paused", GetStreamId());

				this->schedulerStream->MakeInactive();
			}

			AssertIsConsistent();
		}

		void RoundRobinSendQueue::OutgoingStream::Resume()
		{
			MS_TRACE();

			MS_ASSERT(this->pauseState == PauseState::RESETTING, "pause state must be RESETTING");

			this->pauseState = PauseState::NOT_PAUSED;
			this->schedulerStream->MayMakeActive();

			AssertIsConsistent();
		}

		void RoundRobinSendQueue::OutgoingStream::SetAsResetting()
		{
			MS_TRACE();

			MS_ASSERT(this->pauseState == PauseState::PAUSED, "pause state must be PAUSED");

			this->pauseState = PauseState::RESETTING;
		}

		void RoundRobinSendQueue::OutgoingStream::Reset()
		{
			MS_TRACE();

			// This can be called both when an outgoing stream reset has been responded
			// to, or when the entire SendQueue is reset due to detecting the peer having
			// restarted. The stream may be in any state at this time.
			const PauseState oldPauseState = this->pauseState;

			this->pauseState       = PauseState::NOT_PAUSED;
			this->nextOrderedMid   = 0;
			this->nextUnorderedMid = 0;
			this->nextSsn          = 0;

			if (!this->items.empty())
			{
				// If this message has been partially sent, reset it so that it will be
				// re-sent.
				auto& item = this->items.front();

				this->bufferedAmountThresholdWatcher.Increase(
				  item.message.GetPayloadLength() - item.remainingLength);
				this->parent.totalBufferedAmountThresholdWatcher.Increase(
				  item.message.GetPayloadLength() - item.remainingLength);
				item.remainingOffset = 0;
				item.remainingLength = item.message.GetPayloadLength();
				item.mid             = std::nullopt;
				item.ssn             = std::nullopt;
				item.currentFsn      = 0;

				if (oldPauseState == PauseState::PAUSED || oldPauseState == PauseState::RESETTING)
				{
					this->schedulerStream->MayMakeActive();
				}
			}

			AssertIsConsistent();
		}

		bool RoundRobinSendQueue::OutgoingStream::HasPartiallySentMessage() const
		{
			MS_TRACE();

			if (this->items.empty())
			{
				return false;
			}

			return this->items.front().mid.has_value();
		}

		void RoundRobinSendQueue::OutgoingStream::HandleMessageExpired(
		  RoundRobinSendQueue::OutgoingStream::Item& item)
		{
			MS_TRACE();

			this->bufferedAmountThresholdWatcher.Decrease(item.remainingLength);

			this->parent.totalBufferedAmountThresholdWatcher.Decrease(item.remainingLength);

			if (item.attributes.lifecycleId != 0)
			{
				MS_DEBUG_DEV(
				  "triggering OnAssociationLifecycleMessageExpired(%" PRIu64 "), /*maybeDelivered*/ false)",
				  item.attributes.lifecycleId);

				this->parent.associationListener.OnAssociationLifecycleMessageExpired(
				  item.attributes.lifecycleId, /*maybeDelivered*/ false);
				this->parent.associationListener.OnAssociationLifecycleMessageEnd(item.attributes.lifecycleId);
			}
		}

		void RoundRobinSendQueue::OutgoingStream::AssertIsConsistent() const
		{
			MS_TRACE();

			size_t bytes{ 0 };

			for (const auto& item : this->items)
			{
				bytes += item.remainingLength;
			}

			MS_ASSERT(
			  bytes == this->bufferedAmountThresholdWatcher.GetValue(),
			  "bytes != this->bufferedAmountThresholdWatcher.GetValue()");
		}
	} // namespace SCTP
} // namespace RTC
