#define MS_CLASS "RTC::SCTP::ReassemblyQueue"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/rx/ReassemblyQueue.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/rx/InterleavedReassemblyStreams.hpp"
#include "RTC/SCTP/rx/TraditionalReassemblyStreams.hpp"
#include <string>

namespace RTC
{
	namespace SCTP
	{
		ReassemblyQueue::ReassemblyQueue(size_t maxLengthBytes, bool useMessageInterleaving)
		  : maxLengthBytes(maxLengthBytes),
		    watermarkBytes(this->maxLengthBytes * ReassemblyQueue::HighWatermarkLimit),
		    reassemblyStreams(CreateReassemblyStreams(
		      [this](std::span<const Types::UnwrappedTsn> tsns, Message message)
		      {
			      AddReassembledMessage(tsns, std::move(message));
		      },
		      useMessageInterleaving))
		{
			MS_TRACE();
		}

		ReassemblyQueue::~ReassemblyQueue()
		{
			MS_TRACE();
		}

		void ReassemblyQueue::AddData(uint32_t tsn, UserData data)
		{
			MS_TRACE();

			MS_DEBUG_DEV(
			  "added data [tsn:%" PRIu32 ", streamId:%" PRIu16 ", mid:%" PRIu32 ", fsn:%" PRIu32
			  ", type:%s]",
			  tsn,
			  data.GetStreamId(),
			  data.GetMessageId(),
			  data.GetFragmentSequenceNumber(),
			  (data.IsBeginning() && data.IsEnd() ? "complete"
			   : data.IsBeginning()               ? "first"
			   : data.IsEnd()                     ? "last"
			                                      : "middle"));

			const Types::UnwrappedTsn unwrappedTsn = this->tsnUnwrapper.Unwrap(tsn);

			// If a stream reset has been received with a "sender's last assigned tsn"
			// in the future, the association is in "deferred reset processing" mode
			// and must buffer chunks until it's exited.
			if (
			  this->deferredResetStreams.has_value() &&
			  unwrappedTsn > this->deferredResetStreams->senderLastAssignedTsn &&
			  this->deferredResetStreams->streamIds.contains(data.GetStreamId()))
			{
				MS_DEBUG_DEV(
				  "deferrink chunk [tsn:%" PRIu32 ", streamId:%" PRIu16 "] until tsn %" PRIu32,
				  tsn,
				  data.GetStreamId(),
				  this->deferredResetStreams->senderLastAssignedTsn.Wrap());

				// https://tools.ietf.org/html/rfc6525#section-5.2.2
				//
				// "In this mode, any data arriving with a TSN larger than the Sender's
				// Last Assigned TSN for the affected stream(s) MUST be queued locally
				// and held until the cumulative acknowledgment point reaches the
				// Sender's Last Assigned TSN."

				this->queuedBytes += data.GetPayloadLength();

				// NOTE: We use C++20 so we don't support `std::move_only_function` and
				// hence we need to move UserData to a shared pointer. Otherwise it will
				// fail to compile because `std::function` doesn't accept move-only
				// callables and `UserData` has its copy constructor deleted, so the
				// resulting lambda is not copyable and `std::function` will reject it.
				auto sharedData = std::make_shared<UserData>(std::move(data));

				this->deferredResetStreams->deferredActions.emplace_back(
				  [this, tsn, sharedData]() mutable
				  {
					  this->queuedBytes -= sharedData->GetPayloadLength();

					  AddData(tsn, std::move(*sharedData));
				  });

				// TODO: SCTP: Once we upgrade to C++23, replace the above with:
				// this->deferredResetStreams->deferredActions.emplace_back(
				//   [this, tsn, data = std::move(data)]() mutable
				//   {
				// 	  this->queuedBytes -= data.GetPayloadLength();

				// 	  AddData(tsn, std::move(data));
				//   });
			}
			else
			{
				this->queuedBytes += this->reassemblyStreams->AddData(unwrappedTsn, std::move(data));
			}

			// https://datatracker.ietf.org/doc/html/rfc9260#section-6.9
			//
			// "If the data receiver runs out of buffer space while still waiting for
			// more fragments to complete the reassembly of the message, it SHOULD
			// dispatch part of its inbound message through a partial delivery API
			// (see Section 11), freeing some of its receive buffer space so that the
			// rest of the message can be received."

			// TODO: dcsctp: Support EOR flag and partial delivery?

			AssertIsConsistent();
		}

		std::optional<Message> ReassemblyQueue::GetNextMessage()
		{
			MS_TRACE();

			if (this->reassembledMessages.empty())
			{
				return std::nullopt;
			}

			Message message = std::move(this->reassembledMessages.front());

			this->reassembledMessages.pop_front();
			this->queuedBytes -= message.GetPayloadLength();

			return message;
		}

		void ReassemblyQueue::HandleForwardTsn(
		  uint32_t newCumulativeTsn, std::span<const AnyForwardTsnChunk::SkippedStream> skippedStreams)
		{
			MS_TRACE();

			const Types::UnwrappedTsn tsn = this->tsnUnwrapper.Unwrap(newCumulativeTsn);

			if (this->deferredResetStreams.has_value() && tsn > this->deferredResetStreams->senderLastAssignedTsn)
			{
				MS_DEBUG_DEV("forward TSN to %" PRIu32 ", deferring", tsn.Wrap());

				this->deferredResetStreams->deferredActions.emplace_back(
				  [this,
				   newCumulativeTsn,
				   skippedStreams2 = std::vector<AnyForwardTsnChunk::SkippedStream>(
				     skippedStreams.begin(), skippedStreams.end())]
				  {
					  HandleForwardTsn(newCumulativeTsn, skippedStreams2);
				  });

				AssertIsConsistent();

				return;
			}

			MS_DEBUG_DEV("forward TSN to %" PRIu32 ", performing", tsn.Wrap());

			this->queuedBytes -= this->reassemblyStreams->HandleForwardTsn(tsn, skippedStreams);

			AssertIsConsistent();
		}

		void ReassemblyQueue::ResetStreamsAndLeaveDeferredReset(std::span<const uint16_t> streamIds)
		{
			MS_TRACE();

#if MS_LOG_DEV_LEVEL == 3
			std::string streamIdList;

			for (const auto streamId : streamIds)
			{
				if (!streamIdList.empty())
				{
					streamIdList += ',';
				}

				streamIdList += std::to_string(streamId);
			}

			MS_DEBUG_DEV("resetting streams [streamIds:%s]", streamIdList.c_str());
#endif

			// https://tools.ietf.org/html/rfc6525#section-5.2.2
			//
			// "streams MUST be reset to 0 as the next expected SSN."
			this->reassemblyStreams->ResetStreams(streamIds);

			if (this->deferredResetStreams.has_value())
			{
				MS_DEBUG_DEV(
				  "leaving deferred reset processing, feeding back %zu actions",
				  this->deferredResetStreams->deferredActions.size());

				// https://tools.ietf.org/html/rfc6525#section-5.2.2
				//
				// "Any queued TSNs (queued at step E2) MUST now be released and
				// processed normally."
				auto deferredActions = std::move(this->deferredResetStreams->deferredActions);

				this->deferredResetStreams = std::nullopt;

				for (auto& action : deferredActions)
				{
					action();
				}
			}

			AssertIsConsistent();
		}

		void ReassemblyQueue::EnterDeferredReset(
		  uint32_t senderLastAssignedTsn, std::span<const uint16_t> streamIds)
		{
			MS_TRACE();

			if (!this->deferredResetStreams.has_value())
			{
				return;
			}

			MS_DEBUG_DEV(
			  "entering deferred reset [senderLastAssignedTsn:%" PRIu32 "]", senderLastAssignedTsn);

			this->deferredResetStreams = std::make_optional<DeferredResetStreams>(
			  this->tsnUnwrapper.Unwrap(senderLastAssignedTsn),
			  std::set<uint16_t>(streamIds.begin(), streamIds.end()));

			AssertIsConsistent();
		}

		std::unique_ptr<ReassemblyStreamsInterface> ReassemblyQueue::CreateReassemblyStreams(
		  ReassemblyStreamsInterface::OnAssembledMessage onAssembledMessage, bool useMessageInterleaving)
		{
			MS_TRACE();

			if (useMessageInterleaving)
			{
				return std::make_unique<InterleavedReassemblyStreams>(std::move(onAssembledMessage));
			}
			else
			{
				return std::make_unique<TraditionalReassemblyStreams>(std::move(onAssembledMessage));
			}
		}

		void ReassemblyQueue::AddReassembledMessage(std::span<const Types::UnwrappedTsn> tsns, Message message)
		{
			MS_TRACE();

#if MS_LOG_DEV_LEVEL == 3
			std::string tsnList;

			for (const auto tsn : tsns)
			{
				if (!tsnList.empty())
				{
					tsnList += ',';
				}

				tsnList += std::to_string(tsn.Wrap());
			}

			MS_DEBUG_DEV(
			  "resetting streams [ppid:%" PRIu32 ", payloadLength:%zu, tsns:%s]",
			  message.GetPayloadProtocolId(),
			  message.GetPayloadLength(),
			  tsnList.c_str());
#endif

			this->queuedBytes += message.GetPayloadLength();
			this->reassembledMessages.emplace_back(std::move(message));
		}

		void ReassemblyQueue::AssertIsConsistent() const
		{
			MS_TRACE();

			// Allow this->queuedBytes to be larger than this->maxLengthBytes, as it's
			// not actively enforced in this class. But in case it wraps around
			// (becomes negative, but as it's unsigned, that would wrap to very big),
			// this would trigger.
			MS_ASSERT(
			  this->queuedBytes <= 2 * this->maxLengthBytes,
			  "this->queuedBytes > 2 * this->maxLengthBytes");
		}
	} // namespace SCTP
} // namespace RTC
