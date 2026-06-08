#define MS_CLASS "RTC::SCTP::StreamScheduler"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/tx/StreamScheduler.hpp"
#include "Logger.hpp"
#include <ranges>

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		std::optional<SendQueueInterface::DataToSend> StreamScheduler::Produce(uint64_t nowMs, size_t maxLength)
		{
			MS_TRACE();

			// For non-interleaved streams, avoid rescheduling while still sending a
			// message as it needs to be sent in full. For interleaved messaging,
			// reschedule for every I-DATA chunk sent.
			const bool rescheduling = this->enableMessageInterleaving || !this->currentlySendingAMessage;

			MS_DEBUG_DEV("producing data, rescheduling");

			MS_ASSERT(
			  rescheduling || this->currentStream,
			  "it must be rescheduling or there should be current stream");

			std::optional<SendQueueInterface::DataToSend> dataToSend;

			while (!dataToSend.has_value() && !this->activeStreams.empty())
			{
				if (rescheduling)
				{
					const auto it = this->activeStreams.begin();

					this->currentStream = *it;

					MS_DEBUG_DEV("rescheduling to streamId %" PRIu16, this->currentStream->GetStreamId());

					this->activeStreams.erase(it);
					this->currentStream->ForceMarkInactive();
				}
				else
				{
					MS_DEBUG_DEV(
					  "producing from previous streamId %" PRIu16, this->currentStream->GetStreamId());

					MS_ASSERT(
					  std::ranges::any_of(
					    this->activeStreams,
					    [this](const auto* stream)
					    {
						    return stream == this->currentStream;
					    }),
					  "current stream should be in active streams");
				}

				dataToSend = this->currentStream->Produce(nowMs, maxLength);
			}

			if (!dataToSend.has_value())
			{
				MS_DEBUG_DEV("there is no stream with data, cannot produce any data");

				AssertIsConsistent();

				return std::nullopt;
			}

			MS_ASSERT(
			  dataToSend->data.GetStreamId() == this->currentStream->GetStreamId(),
			  "no matching stream id");

			MS_DEBUG_DEV(
			  "producing data [type:%s, beginning:%s, end:%s, streamId:%" PRIu16 ", ppid:%" PRIu32
			  ", length:%zu]",
			  dataToSend->data.IsUnordered() ? "unordered" : "ordered",
			  dataToSend->data.IsBeginning() ? "yes" : "no",
			  dataToSend->data.IsEnd() ? "yes" : "no",
			  dataToSend->data.GetStreamId(),
			  dataToSend->data.GetPayloadProtocolId(),
			  dataToSend->data.GetPayloadLength());

			this->currentlySendingAMessage = !dataToSend->data.IsEnd();
			this->virtualTime              = this->currentStream->GetCurrentTime();

			// One side-effect of rescheduling is that the new stream will not be
			// present in `this->activeStreams`.
			const size_t bytesToSendNext = this->currentStream->GetBytesToSendInNextMessage();

			if (rescheduling && bytesToSendNext > 0)
			{
				this->currentStream->MakeActive(bytesToSendNext);
			}
			else if (!rescheduling && bytesToSendNext == 0)
			{
				this->currentStream->MakeInactive();
			}

			AssertIsConsistent();

			return dataToSend;
		}

		std::set<uint16_t> StreamScheduler::GetActiveStreamsForTesting() const
		{
			MS_TRACE();

			std::set<uint16_t> streamIds;

			for (const auto& stream : this->activeStreams)
			{
				streamIds.insert(stream->GetStreamId());
			}

			return streamIds;
		}

		void StreamScheduler::AssertIsConsistent() const
		{
			MS_TRACE();

			for (const Stream* stream : this->activeStreams)
			{
				if (stream->GetNextFinishTime() == 0)
				{
					MS_ASSERT(
					  stream->GetNextFinishTime() > 0,
					  "stream %" PRIu16 " is active but has no next-finish-time",
					  stream->GetStreamId());
				}
			}
		}

		void StreamScheduler::Stream::SetPriority(uint16_t priority)
		{
			MS_TRACE();

			this->priority      = priority;
			this->inverseWeight = 1.0 / std::max(static_cast<double>(priority), 1e-6);
		}

		void StreamScheduler::Stream::MayMakeActive()
		{
			MS_TRACE();

			MS_ASSERT(this->nextFinishTime == 0, "next-finish-time must be 0");

			const size_t bytesToSendNext = GetBytesToSendInNextMessage();

			if (bytesToSendNext == 0)
			{
				return;
			}

			MakeActive(bytesToSendNext);
		}

		void StreamScheduler::Stream::MakeActive(size_t bytesToSendNext)
		{
			MS_TRACE();

			this->currentVirtualTime = this->parent.virtualTime;

			MS_ASSERT(bytesToSendNext > 0, "bytesToSendNext must be higher than 0");

			const double nextFinishTime =
			  CalculateFinishTime(std::min(bytesToSendNext, this->parent.maxPayloadBytes));

			MS_ASSERT(nextFinishTime > 0, "nextFinishTime must be higher than 0");

			MS_DEBUG_DEV(
			  "making streamId %" PRIu16 " active, expiring at %f", GetStreamId(), nextFinishTime);

			MS_ASSERT(this->nextFinishTime == 0, "this->nextFinishTime must be 0");

			this->nextFinishTime = nextFinishTime;

			MS_ASSERT(
			  !std::ranges::any_of(
			    this->parent.activeStreams,
			    [this](const auto* stream)
			    {
				    return stream == this;
			    }),
			  "this stream must not be in active streams");

			this->parent.activeStreams.emplace(this);
		}

		void StreamScheduler::Stream::ForceMarkInactive()
		{
			MS_TRACE();

			MS_DEBUG_DEV("making streamId %" PRIu16 " inactive", GetStreamId());

			MS_ASSERT(this->nextFinishTime != 0, "this->nextFinishTime must be different than 0");

			this->nextFinishTime = 0;
		}

		void StreamScheduler::Stream::MakeInactive()
		{
			MS_TRACE();

			ForceMarkInactive();

			std::erase_if(
			  this->parent.activeStreams,
			  [this](const auto* stream)
			  {
				  return stream == this;
			  });
		}

		double StreamScheduler::Stream::CalculateFinishTime(size_t bytesToSendNext) const
		{
			MS_TRACE();

			if (this->parent.enableMessageInterleaving)
			{
				// Perform weighted fair queuing scheduling.
				return this->currentVirtualTime +
				       (static_cast<double>(bytesToSendNext) * this->inverseWeight);
			}

			// Perform round-robin scheduling by letting the stream have its next
			// virtual finish time in the future. It doesn't matter how far into the
			// future, just any positive number so that any other stream that has the
			// same virtual finish time as this stream gets to produce their data
			// before revisiting this stream.
			return this->currentVirtualTime + 1;
		}

		std::optional<SendQueueInterface::DataToSend> StreamScheduler::Stream::Produce(
		  uint64_t nowMs, size_t maxLength)
		{
			MS_TRACE();

			std::optional<SendQueueInterface::DataToSend> dataToSend =
			  this->producer.Produce(nowMs, maxLength);

			if (dataToSend.has_value())
			{
				const double newCurrentVirtualTime = CalculateFinishTime(dataToSend->data.GetPayloadLength());

				MS_DEBUG_DEV(
				  "virtual time changed [from:%f, to:%f]", this->currentVirtualTime, newCurrentVirtualTime);

				this->currentVirtualTime = newCurrentVirtualTime;
			}

			return dataToSend;
		}
	} // namespace SCTP
} // namespace RTC
