#define MS_CLASS "RTC::SCTP::TraditionalReassemblyStreams"
// TODO: SCTP: COMMENT
#define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/rx/TraditionalReassemblyStreams.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		TraditionalReassemblyStreams::TraditionalReassemblyStreams(
		  ReassemblyStreamsInterface::OnAssembledMessage onAssembledMessage)
		  : onAssembledMessage(std::move(onAssembledMessage))
		{
			MS_TRACE();
		}

		int32_t TraditionalReassemblyStreams::AddData(Types::UnwrappedTsn tsn, UserData data)
		{
			MS_TRACE();

			if (data.IsUnordered())
			{
				const auto it = this->unorderedStreams.try_emplace(data.GetStreamId(), this).first;

				auto& stream = it->second;

				return stream.AddData(tsn, std::move(data));
			}

			const auto it = this->orderedStreams.try_emplace(data.GetStreamId(), this).first;

			auto& stream = it->second;

			return stream.AddData(tsn, std::move(data));
		}

		size_t TraditionalReassemblyStreams::HandleForwardTsn(
		  Types::UnwrappedTsn newCumulativeTsn,
		  std::span<const AnyForwardTsnChunk::SkippedStream> skippedStreams)
		{
			MS_TRACE();

			size_t removedBytes = 0;

			// The `skippedStreams` only cover ordered messages - need to iterate all
			// unordered streams manually to remove those chunks.
			for (auto& [unused, stream] : this->unorderedStreams)
			{
				removedBytes += stream.EraseTo(newCumulativeTsn);
			}

			for (const auto& skippedStream : skippedStreams)
			{
				const auto it = this->orderedStreams.try_emplace(skippedStream.streamId, this).first;

				auto& stream = it->second;

				removedBytes += stream.EraseTo(skippedStream.ssn);
			}

			return removedBytes;
		}

		void TraditionalReassemblyStreams::ResetStreams(std::span<const uint16_t> streamIds)
		{
			MS_TRACE();

			if (streamIds.empty())
			{
				for (auto& [streamId, stream] : this->orderedStreams)
				{
					MS_DEBUG_DEV("resetting implicit stream [streamId:%" PRIu16 "]", streamId);

					stream.Reset();
				}
			}
			else
			{
				for (const auto streamId : streamIds)
				{
					const auto it = this->orderedStreams.find(streamId);

					if (it != this->orderedStreams.end())
					{
						MS_DEBUG_DEV("resetting explicit stream [streamId:%" PRIu16 "]", streamId);

						auto& stream = it->second;

						stream.Reset();
					}
				}
			}
		}

		size_t TraditionalReassemblyStreams::StreamBase::AssembleMessage(
		  const ChunkMap::iterator start, const ChunkMap::iterator end)
		{
			MS_TRACE();

			const size_t count = std::distance(start, end);

			// Fast path - zero-copy
			if (count == 1)
			{
				return AssembleMessage(start->first, std::move(start->second));
			}

			// Slow path - will need to concatenate the payload.
			std::vector<Types::UnwrappedTsn> tsns;
			std::vector<uint8_t> payload;

			const size_t payloadLength = std::accumulate(
			  start,
			  end,
			  0,
			  [](size_t acc, const auto& i)
			  {
				  const auto& data = i.second;

				  return acc + data.GetPayloadLength();
			  });

			tsns.reserve(count);
			payload.reserve(payloadLength);

			for (auto it = start; it != end; ++it)
			{
				const auto tsn = it->first;

				auto& data = it->second;

				tsns.push_back(tsn);
				payload.insert(payload.end(), data.GetPayload().begin(), data.GetPayload().end());
			}

			const auto& startData = start->second;

			Message message(startData.GetStreamId(), startData.GetPayloadProtocolId(), std::move(payload));

			this->parent.onAssembledMessage(tsns, std::move(message));

			return payloadLength;
		}

		size_t TraditionalReassemblyStreams::StreamBase::AssembleMessage(
		  Types::UnwrappedTsn tsn, UserData data)
		{
			MS_TRACE();

			// Fast path - zero-copy.
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

		int32_t TraditionalReassemblyStreams::OrderedStream::AddData(Types::UnwrappedTsn tsn, UserData data)
		{
			MS_TRACE();

			const auto queuedBytes        = static_cast<int32_t>(data.GetPayloadLength());
			const Types::UnwrappedSsn ssn = this->ssnUnwrapper.Unwrap(data.GetStreamSequenceNumber());

			if (ssn == this->nextSsn)
			{
				return queuedBytes - TryToAssembleMessagesFastpath(ssn, tsn, std::move(data));
			}

			const auto [it, inserted] = this->chunksBySsn[ssn].emplace(tsn, std::move(data));

			if (!inserted)
			{
				return 0;
			}

			return queuedBytes;
		}

		size_t TraditionalReassemblyStreams::OrderedStream::EraseTo(uint16_t ssn)
		{
			MS_TRACE();

			Types::UnwrappedSsn unwrappedSsn = this->ssnUnwrapper.Unwrap(ssn);

			const auto endIt = this->chunksBySsn.upper_bound(unwrappedSsn);

			size_t removedBytes = std::accumulate(
			  this->chunksBySsn.begin(),
			  endIt,
			  0,
			  [](size_t acc1, const auto& i1)
			  {
				  return acc1 + std::accumulate(
				                  i1.second.begin(),
				                  i1.second.end(),
				                  0,
				                  [](size_t acc2, const auto& i2)
				                  {
					                  const auto& data = i2.second;

					                  return acc2 + data.GetPayloadLength();
				                  });
			  });

			this->chunksBySsn.erase(this->chunksBySsn.begin(), endIt);

			if (unwrappedSsn >= this->nextSsn)
			{
				unwrappedSsn.Increment();

				this->nextSsn = unwrappedSsn;
			}

			removedBytes += TryToAssembleMessages();

			return removedBytes;
		}

		size_t TraditionalReassemblyStreams::OrderedStream::TryToAssembleMessage()
		{
			MS_TRACE();

			if (this->chunksBySsn.empty() || this->chunksBySsn.begin()->first != this->nextSsn)
			{
				return 0;
			}

			ChunkMap& chunks = this->chunksBySsn.begin()->second;

			if (!chunks.begin()->second.IsBeginning() || !chunks.rbegin()->second.IsEnd())
			{
				return 0;
			}

			const uint32_t tsnDiff =
			  Types::UnwrappedTsn::Difference(chunks.rbegin()->first, chunks.begin()->first);

			if (tsnDiff != chunks.size() - 1)
			{
				return 0;
			}

			const size_t assembledBytes = AssembleMessage(chunks.begin(), chunks.end());

			this->chunksBySsn.erase(this->chunksBySsn.begin());
			this->nextSsn.Increment();

			return assembledBytes;
		}

		size_t TraditionalReassemblyStreams::OrderedStream::TryToAssembleMessages()
		{
			MS_TRACE();

			size_t assembledBytes = 0;

			for (;;)
			{
				const size_t assembledBytesThisIter = TryToAssembleMessage();

				if (assembledBytesThisIter == 0)
				{
					break;
				}

				assembledBytes += assembledBytesThisIter;
			}

			return assembledBytes;
		}

		size_t TraditionalReassemblyStreams::OrderedStream::TryToAssembleMessagesFastpath(
		  Types::UnwrappedSsn ssn, Types::UnwrappedTsn tsn, UserData data)
		{
			MS_TRACE();

			MS_ASSERT(ssn == this->nextSsn, "ssn != this->nextSsn");

			size_t assembledBytes = 0;

			if (data.IsBeginning() && data.IsEnd())
			{
				assembledBytes += AssembleMessage(tsn, std::move(data));

				this->nextSsn.Increment();
			}
			else
			{
				const size_t queuedBytes = data.GetPayloadLength();

				auto [it, inserted] = this->chunksBySsn[ssn].emplace(tsn, std::move(data));

				// Not actually assembled, but deduplicated meaning queued size doesn't
				// include this message.
				if (!inserted)
				{
					return queuedBytes;
				}
			}

			return assembledBytes + TryToAssembleMessages();
		}

		int32_t TraditionalReassemblyStreams::UnorderedStream::AddData(Types::UnwrappedTsn tsn, UserData data)
		{
			MS_TRACE();

			// Fastpath for already assembled chunks.
			if (data.IsBeginning() && data.IsEnd())
			{
				AssembleMessage(tsn, std::move(data));

				return 0;
			}

			auto queuedBytes = static_cast<int32_t>(data.GetPayloadLength());

			const auto [it, inserted] = this->chunks.emplace(tsn, std::move(data));

			if (!inserted)
			{
				return 0;
			}

			queuedBytes -= TryToAssembleMessage(it);

			return queuedBytes;
		}

		size_t TraditionalReassemblyStreams::UnorderedStream::EraseTo(Types::UnwrappedTsn tsn)
		{
			MS_TRACE();

			const auto endIt = this->chunks.upper_bound(tsn);

			const size_t removedBytes = std::accumulate(
			  this->chunks.begin(),
			  endIt,
			  0,
			  [](size_t acc, const auto& i)
			  {
				  const auto& data = i.second;

				  return acc + data.GetPayloadLength();
			  });

			this->chunks.erase(this->chunks.begin(), endIt);

			return removedBytes;
		}

		size_t TraditionalReassemblyStreams::UnorderedStream::TryToAssembleMessage(ChunkMap::iterator it)
		{
			MS_TRACE();

			// TODO: dcsctp: This method is O(N) with the number of fragments in a
			// message, which can be inefficient for very large values of N. This
			// could be optimized by e.g. only trying to assemble a message once _any_
			// beginning and _any_ end has been found.
			const std::optional<ChunkMap::iterator> start = FindBeginning(it);

			if (!start.has_value())
			{
				return 0;
			}

			const std::optional<ChunkMap::iterator> end = FindEnd(it);

			if (!end.has_value())
			{
				return 0;
			}

			const size_t bytesAssembled = AssembleMessage(*start, *end);

			this->chunks.erase(*start, *end);

			return bytesAssembled;
		}

		std::optional<std::map<Types::UnwrappedTsn, UserData>::iterator> TraditionalReassemblyStreams::
		  UnorderedStream::FindBeginning(std::map<Types::UnwrappedTsn, UserData>::iterator it)
		{
			MS_TRACE();

			Types::UnwrappedTsn prevTsn = it->first;

			for (;;)
			{
				if (it->second.IsBeginning())
				{
					return it;
				}

				if (it == this->chunks.begin())
				{
					return std::nullopt;
				}

				it--;

				if (it->first.GetNextValue() != prevTsn)
				{
					return std::nullopt;
				}

				prevTsn = it->first;
			}
		}

		std::optional<std::map<Types::UnwrappedTsn, UserData>::iterator> TraditionalReassemblyStreams::
		  UnorderedStream::FindEnd(std::map<Types::UnwrappedTsn, UserData>::iterator it)
		{
			MS_TRACE();

			Types::UnwrappedTsn prevTsn = it->first;

			for (;;)
			{
				if (it->second.IsEnd())
				{
					return ++it;
				}

				it++;

				if (it == this->chunks.end())
				{
					return std::nullopt;
				}

				if (it->first != prevTsn.GetNextValue())
				{
					return std::nullopt;
				}

				prevTsn = it->first;
			}
		}
	} // namespace SCTP
} // namespace RTC
