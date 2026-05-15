#define MS_CLASS "RTC::SCTP::InterleavedReassemblyStreams"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/rx/InterleavedReassemblyStreams.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		InterleavedReassemblyStreams::InterleavedReassemblyStreams(
		  ReassemblyStreamsInterface::OnAssembledMessage onAssembledMessage)
		  : onAssembledMessage(std::move(onAssembledMessage))
		{
			MS_TRACE();
		}

		int32_t InterleavedReassemblyStreams::AddData(Types::UnwrappedTsn tsn, UserData data)
		{
			MS_TRACE();

			return GetOrCreateStream(FullStreamId(data.IsUnordered(), data.GetStreamId()))
			  .AddData(tsn, std::move(data));
		}

		size_t InterleavedReassemblyStreams::HandleForwardTsn(
		  Types::UnwrappedTsn /*newCumulativeTsn*/,
		  std::span<const AnyForwardTsnChunk::SkippedStream> skippedStreams)
		{
			MS_TRACE();

			size_t bytesRemoved = 0;

			for (const auto& skippedStream : skippedStreams)
			{
				bytesRemoved +=
				  GetOrCreateStream(FullStreamId(skippedStream.unordered, skippedStream.streamId))
				    .EraseTo(skippedStream.mid);
			}

			return bytesRemoved;
		}

		void InterleavedReassemblyStreams::ResetStreams(std::span<const uint16_t> streamIds)
		{
			MS_TRACE();

			if (streamIds.empty())
			{
				for (auto& [fullStreamId, stream] : this->streams)
				{
					MS_DEBUG_DEV("resetting implicit stream [streamId:%" PRIu16 "]", fullStreamId.streamId);

					stream.Reset();
				}
			}
			else
			{
				for (const auto streamId : streamIds)
				{
					MS_DEBUG_DEV("resetting explicit stream [streamId:%" PRIu16 "]", streamId);

					GetOrCreateStream(FullStreamId(/*unordered*/ true, streamId)).Reset();
					GetOrCreateStream(FullStreamId(/*unordered*/ false, streamId)).Reset();
				}
			}
		}

		InterleavedReassemblyStreams::Stream& InterleavedReassemblyStreams::GetOrCreateStream(
		  const FullStreamId& streamId)
		{
			MS_TRACE();

			auto it = this->streams.find(streamId);

			if (it == this->streams.end())
			{
				it = this->streams
				       .emplace(
				         std::piecewise_construct,
				         std::forward_as_tuple(streamId),
				         std::forward_as_tuple(streamId, this))
				       .first;
			}

			auto& stream = it->second;

			return stream;
		}

		int32_t InterleavedReassemblyStreams::Stream::AddData(Types::UnwrappedTsn tsn, UserData data)
		{
			MS_TRACE();

			MS_ASSERT(
			  data.IsUnordered() == this->fullStreamId.unordered,
			  "data.IsUnordered() != this->streamId.unordered");
			MS_ASSERT(
			  data.GetStreamId() == this->fullStreamId.streamId,
			  "data.GetStreamId() != this->fullStreamId.streamId");

			auto queuedBytes = static_cast<int32_t>(data.GetPayloadLength());

			const Types::UnwrappedMid mid = this->midUnwrapper.Unwrap(data.GetMessageId());
			const uint32_t fsn            = data.GetFragmentSequenceNumber();

			// Avoid inserting it into any map if it can be delivered directly.
			if (this->fullStreamId.unordered && data.IsBeginning() && data.IsEnd())
			{
				AssembleMessage(tsn, std::move(data));
				return 0;
			}
			else if (!this->fullStreamId.unordered && mid == this->nextMid && data.IsBeginning() && data.IsEnd())
			{
				AssembleMessage(tsn, std::move(data));

				this->nextMid.Increment();

				// This might unblock assembling more messages.
				return -TryToAssembleMessages();
			}

			// Slow path.
			const auto [unused, inserted] =
			  this->chunksByMid[mid].emplace(fsn, std::make_pair(tsn, std::move(data)));

			if (!inserted)
			{
				return 0;
			}

			if (this->fullStreamId.unordered)
			{
				queuedBytes -= TryToAssembleMessage(mid);
			}
			else
			{
				if (mid == this->nextMid)
				{
					queuedBytes -= TryToAssembleMessages();
				}
			}

			return queuedBytes;
		}

		size_t InterleavedReassemblyStreams::Stream::EraseTo(uint32_t mid)
		{
			MS_TRACE();

			const Types::UnwrappedMid unwrappedMid = this->midUnwrapper.Unwrap(mid);
			size_t removedBytes                    = 0;

			auto it = this->chunksByMid.begin();

			while (it != this->chunksByMid.end() && it->first <= unwrappedMid)
			{
				removedBytes += std::accumulate(
				  it->second.begin(),
				  it->second.end(),
				  0,
				  [](size_t acc, const auto& i)
				  {
					  const auto& data = i.second.second;

					  return acc + data.GetPayloadLength();
				  });

				it = this->chunksByMid.erase(it);
			}

			if (!this->fullStreamId.unordered)
			{
				// For ordered streams, erasing a message might suddenly unblock that
				// queue and allow it to deliver any following received messages.
				if (unwrappedMid >= this->nextMid)
				{
					this->nextMid = unwrappedMid.GetNextValue();
				}

				removedBytes += TryToAssembleMessages();
			}

			return removedBytes;
		}

		size_t InterleavedReassemblyStreams::Stream::TryToAssembleMessage(Types::UnwrappedMid mid)
		{
			MS_TRACE();

			const auto it = this->chunksByMid.find(mid);

			if (it == this->chunksByMid.end())
			{
				MS_DEBUG_DEV("trying to assemble message [mid:%" PRIu32 "]", mid.Wrap());

				return 0;
			}

			ChunkMap& chunks = it->second;

			if (!chunks.begin()->second.second.IsBeginning() || !chunks.rbegin()->second.second.IsEnd())
			{
				MS_DEBUG_DEV(
				  "cannot assemble message, missing beggining or end [mid:%" PRIu32 "]", mid.Wrap());

				return 0;
			}

			// NOTE: This is uint32_t - uint32_t casted to uint64_t. Not very nice but
			// it works.
			const int64_t fnsDiff = chunks.rbegin()->first - chunks.begin()->first;

			if (fnsDiff != (static_cast<int64_t>(chunks.size()) - 1))
			{
				MS_DEBUG_DEV(
				  "cannot assemble message, not all chunks exist [has:%zu, expected:%" PRIi64 "]",
				  chunks.size(),
				  fnsDiff + 1);

				return 0;
			}

			const size_t removedBytes = AssembleMessage(chunks);

			MS_DEBUG_DEV("message assembled [mid:%" PRIu32 ", removedBytes:%zu]", mid.Wrap(), removedBytes);

			this->chunksByMid.erase(mid);

			return removedBytes;
		}

		size_t InterleavedReassemblyStreams::Stream::TryToAssembleMessages()
		{
			MS_TRACE();

			size_t removedBytes = 0;

			for (;;)
			{
				const size_t removedBytesThisIt = TryToAssembleMessage(this->nextMid);

				if (removedBytesThisIt == 0)
				{
					break;
				}

				removedBytes += removedBytesThisIt;

				this->nextMid.Increment();
			}

			return removedBytes;
		}

		size_t InterleavedReassemblyStreams::Stream::AssembleMessage(Types::UnwrappedTsn tsn, UserData data)
		{
			MS_TRACE();

			const size_t payloadLength        = data.GetPayloadLength();
			const auto streamId               = data.GetStreamId();
			const auto pip                    = data.GetPayloadProtocolId();
			const Types::UnwrappedTsn tsns[1] = { tsn };

			Message message(
			  streamId,
			  pip,
			  // NOTE: clang-tidy doesn't understand that this is fine.
			  // NOLINTNEXTLINE(bugprone-use-after-move, hicpp-invalid-access-moved)
			  std::move(data).ReleasePayload());

			this->parent.onAssembledMessage(tsns, std::move(message));

			return payloadLength;
		}

		size_t InterleavedReassemblyStreams::Stream::AssembleMessage(ChunkMap& tsnChunks)
		{
			MS_TRACE();

			const size_t count = tsnChunks.size();

			// Fast path - zero-copy.
			if (count == 1)
			{
				return AssembleMessage(
				  tsnChunks.begin()->second.first, std::move(tsnChunks.begin()->second.second));
			}

			// Slow path - will need to concatenate the payload.
			std::vector<Types::UnwrappedTsn> tsns;
			std::vector<uint8_t> payload;

			const size_t payloadLength = std::accumulate(
			  tsnChunks.begin(),
			  tsnChunks.end(),
			  0,
			  [](size_t acc, const auto& i)
			  {
				  const auto& data = i.second.second;

				  return acc + data.GetPayloadLength();
			  });

			payload.reserve(payloadLength);
			tsns.reserve(count);

			for (auto& item : tsnChunks)
			{
				const Types::UnwrappedTsn tsn = item.second.first;

				UserData& data = item.second.second;

				tsns.push_back(tsn);
				payload.insert(payload.end(), data.GetPayload().begin(), data.GetPayload().end());
			}

			const UserData& data = tsnChunks.begin()->second.second;

			Message message(data.GetStreamId(), data.GetPayloadProtocolId(), std::move(payload));

			this->parent.onAssembledMessage(tsns, std::move(message));

			return payloadLength;
		}
	} // namespace SCTP
} // namespace RTC
