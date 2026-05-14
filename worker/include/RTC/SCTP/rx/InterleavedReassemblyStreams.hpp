#ifndef MS_RTC_SCTP_INTERLEAVED_REASSEMBLY_STREAMS_HPP
#define MS_RTC_SCTP_INTERLEAVED_REASSEMBLY_STREAMS_HPP

#include "common.hpp"
#include "RTC/SCTP/rx/ReassemblyStreamsInterface.hpp"
#include <map>
#include <span>
#include <tuple> // std::tie()

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Handles reassembly of incoming data when interleaved message sending is
		 * enabled on the association, i.e. when RFC 8260 is in use.
		 *
		 * In other words, this class handles data received via I-DATA chunks.
		 */
		class InterleavedReassemblyStreams : public ReassemblyStreamsInterface
		{
		private:
			struct FullStreamId
			{
				FullStreamId(bool unordered, uint16_t streamId) : unordered(unordered), streamId(streamId)
				{
				}

				friend bool operator<(FullStreamId a, FullStreamId b)
				{
					return std::tie(a.unordered, a.streamId) < std::tie(b.unordered, b.streamId);
				}

				const bool unordered;
				const uint16_t streamId;
			};

		private:
			class Stream
			{
			private:
				using ChunkMap = std::map<uint32_t /*fsn*/, std::pair<Types::UnwrappedTsn, UserData>>;

			public:
				Stream(FullStreamId fullStreamId, InterleavedReassemblyStreams* parent, uint32_t nextMid = 0)
				  : fullStreamId(fullStreamId), parent(*parent), nextMid(midUnwrapper.Unwrap(nextMid))
				{
				}

			public:
				int32_t AddData(Types::UnwrappedTsn tsn, UserData data);

				size_t EraseTo(uint32_t mid);

				void Reset()
				{
					this->midUnwrapper.Reset();
					this->nextMid = this->midUnwrapper.Unwrap(0);
				}

				bool HasUnassembledChunks() const
				{
					return !this->chunksByMid.empty();
				}

			private:
				/**
				 * Try to assemble one message identified by `mid`. Returns the number
				 * of bytes assembled if a message was assembled.
				 */
				size_t TryToAssembleMessage(Types::UnwrappedMid mid);

				/**
				 * Try to assemble several messages in order from the stream. Returns
				 * the number of bytes assembled if a message was assembled.
				 */
				size_t TryToAssembleMessages();

				size_t AssembleMessage(ChunkMap& tsnChunks);

				size_t AssembleMessage(Types::UnwrappedTsn tsn, UserData data);

			private:
				const FullStreamId fullStreamId;
				InterleavedReassemblyStreams& parent;
				std::map<Types::UnwrappedMid, ChunkMap> chunksByMid;
				Types::UnwrappedMid::Unwrapper midUnwrapper;
				Types::UnwrappedMid nextMid;
			};

		public:
			explicit InterleavedReassemblyStreams(
			  ReassemblyStreamsInterface::OnAssembledMessage onAssembledMessage);

		public:
			int32_t AddData(Types::UnwrappedTsn tsn, UserData data) override;

			size_t HandleForwardTsn(
			  Types::UnwrappedTsn newCumulativeAckTsn,
			  std::span<const AnyForwardTsnChunk::SkippedStream> skippedStreams) override;

			void ResetStreams(std::span<const uint16_t> streamIds) override;

		private:
			Stream& GetOrCreateStream(const FullStreamId& streamId);

		private:
			// Callback for when a message has been assembled.
			const OnAssembledMessage onAssembledMessage;
			// All unordered and ordered streams, managing not-yet-assembled data.
			std::map<FullStreamId, Stream> streams;
		};
	} // namespace SCTP
} // namespace RTC

#endif
