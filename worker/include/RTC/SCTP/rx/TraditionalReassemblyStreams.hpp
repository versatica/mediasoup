#ifndef MS_RTC_SCTP_TRADITIONAL_REASSEMBLY_STREAMS_HPP
#define MS_RTC_SCTP_TRADITIONAL_REASSEMBLY_STREAMS_HPP

#include "common.hpp"
#include "RTC/SCTP/rx/ReassemblyStreamsInterface.hpp"
#include <map>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Handles reassembly of incoming data when interleaved message sending is
		 * not enabled on the association, i.e. when RFC 8260 is not in use and
		 * RFC 9260 is to be followed.
		 *
		 * In other words, this class handles data received via DATA chunks.
		 */
		class TraditionalReassemblyStreams : public ReassemblyStreamsInterface
		{
		private:
			using ChunkMap = std::map<Types::UnwrappedTsn, UserData>;

		private:
			/**
			 * Base class for `UnorderedStream` and `OrderedStream` classes.
			 */
			class StreamBase
			{
			protected:
				explicit StreamBase(TraditionalReassemblyStreams* parent) : parent(*parent)
				{
				}

				size_t AssembleMessage(ChunkMap::iterator start, ChunkMap::iterator end);

				size_t AssembleMessage(Types::UnwrappedTsn tsn, UserData data);

			protected:
				TraditionalReassemblyStreams& parent;
			};

		private:
			/**
			 * Manages all received data for a specific ordered stream, and assembles
			 * messages when possible.
			 */
			class OrderedStream : StreamBase
			{
			public:
				explicit OrderedStream(TraditionalReassemblyStreams* parent, uint16_t nextSsn = 0)
				  : StreamBase(parent), nextSsn(ssnUnwrapper.Unwrap(nextSsn))
				{
				}

				int32_t AddData(Types::UnwrappedTsn tsn, UserData data);

				size_t EraseTo(uint16_t ssn);

				void Reset()
				{
					this->ssnUnwrapper.Reset();
					this->nextSsn = this->ssnUnwrapper.Unwrap(uint16_t(0));
				}

				uint16_t GetNextSsn() const
				{
					return this->nextSsn.Wrap();
				}

				bool HasUnassembledChunks() const
				{
					return !chunksBySsn.empty();
				}

			private:
				/**
				 * Try to assemble one message in order from the stream. Returns the
				 * number of bytes assembled if a message was assembled.
				 */
				size_t TryToAssembleMessage();

				/**
				 * Try to assemble several messages in order from the stream. Returns
				 * the number of bytes assembled if a message was assembled.
				 */
				size_t TryToAssembleMessages();

				/**
				 * Same as above but when inserting the first complete message avoid
				 * insertion into the map.
				 */
				size_t TryToAssembleMessagesFastpath(
				  Types::UnwrappedSsn ssn, Types::UnwrappedTsn tsn, UserData data);

			private:
				// NOTE: This must be an ordered container to be able to iterate in SSN
				// order.
				std::map<Types::UnwrappedSsn, ChunkMap> chunksBySsn;
				Types::UnwrappedSsn::Unwrapper ssnUnwrapper;
				Types::UnwrappedSsn nextSsn;
			};

		private:
			/**
			 * Manages all received data for a specific unordered stream, and assembles
			 *  messages when possible.
			 */
			class UnorderedStream : StreamBase
			{
			public:
				explicit UnorderedStream(TraditionalReassemblyStreams* parent) : StreamBase(parent)
				{
				}

				int32_t AddData(Types::UnwrappedTsn tsn, UserData data);

				/**
				 * Returns the number of bytes removed from the queue.
				 */
				size_t EraseTo(Types::UnwrappedTsn tsn);

				bool HasUnassembledChunks() const
				{
					return !this->chunks.empty();
				}

			private:
				/**
				 * Given an iterator to any chunk within the map, try to assemble a
				 * message containing it and - if successful - erase those chunks from
				 * the stream chunks map.
				 *
				 * Returns the number of bytes that were assembled.
				 */
				size_t TryToAssembleMessage(ChunkMap::iterator it);

				/**
				 * Given a map (`chunks`) and an iterator to within that map (`it`),
				 * this method will return an iterator to the first chunk in that
				 * message, which has the `isBeginning` flag set. If there are any gaps,
				 * or if the beginning can't be found, `std::nullopt` is returned.
				 */
				std::optional<std::map<Types::UnwrappedTsn, UserData>::iterator> FindBeginning(
				  std::map<Types::UnwrappedTsn, UserData>::iterator it);

				/**
				 * Given a map (`chunks`) and an iterator to within that map (`it`),
				 * this method will return an iterator to the chunk after the last chunk
				 * in that message, which has the `isEnd` flag set. If there are any
				 * gaps, or if the end can't be found, `std::nullopt` is returned.
				 */
				std::optional<std::map<Types::UnwrappedTsn, UserData>::iterator> FindEnd(
				  std::map<Types::UnwrappedTsn, UserData>::iterator it);

			private:
				ChunkMap chunks;
			};

		public:
			explicit TraditionalReassemblyStreams(
			  ReassemblyStreamsInterface::OnAssembledMessage onAssembledMessage);

		public:
			int32_t AddData(Types::UnwrappedTsn tsn, UserData data) override;

			size_t HandleForwardTsn(
			  Types::UnwrappedTsn newCumulativeAckTsn,
			  std::span<const AnyForwardTsnChunk::SkippedStream> skippedStreams) override;

			void ResetStreams(std::span<const uint16_t> streamIds) override;

		private:
			// Callback for when a message has been assembled.
			const OnAssembledMessage onAssembledMessage;
			// All unordered and ordered streams, managing not-yet-assembled data.
			std::map<uint16_t, UnorderedStream> unorderedStreams;
			std::map<uint16_t, OrderedStream> orderedStreams;
		};
	} // namespace SCTP
} // namespace RTC

#endif
