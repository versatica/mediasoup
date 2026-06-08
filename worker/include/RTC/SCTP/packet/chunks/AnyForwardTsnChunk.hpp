#ifndef MS_RTC_SCTP_ANY_FORWARD_TSN_CHUNK_HPP
#define MS_RTC_SCTP_ANY_FORWARD_TSN_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include <ostream>
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Base class for ForwardTsnChunk and IForwardTsnChunk.
		 */

		class AnyForwardTsnChunk : public Chunk
		{
		public:
			struct SkippedStream
			{
				SkippedStream(uint16_t streamId, uint16_t ssn)
				  : streamId(streamId), ssn(ssn), mid(0), unordered(false)
				{
				}

				SkippedStream(bool unordered, uint16_t streamId, uint32_t mid)
				  : streamId(streamId), ssn(0), mid(mid), unordered(unordered)
				{
				}

				uint16_t streamId;

				/**
				 * Only set for FORWARD-TSN.
				 */
				uint16_t ssn;

				/**
				 * Only set for I-FORWARD-TSN.
				 */
				uint32_t mid;

				/**
				 * Only set for I-FORWARD-TSN.
				 */
				bool unordered;

				bool operator==(const SkippedStream& other) const
				{
					return streamId == other.streamId && ssn == other.ssn && mid == other.mid &&
					       unordered == other.unordered;
				}
			};

		protected:
			AnyForwardTsnChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
			{
			}

		public:
			virtual uint32_t GetNewCumulativeTsn() const = 0;

			virtual uint16_t GetNumberOfSkippedStreams() const = 0;

			virtual std::vector<AnyForwardTsnChunk::SkippedStream> GetSkippedStreams() const = 0;
		};

		/**
		 * For logging purposes in Catch2 tests.
		 */
		inline std::ostream& operator<<(std::ostream& os, const AnyForwardTsnChunk::SkippedStream& s)
		{
			return os << "{streamId:" << s.streamId << ", ssn:" << s.ssn << ", mid:" << s.mid
			          << ", unordered:" << s.unordered << "}";
		}
	} // namespace SCTP
} // namespace RTC

#endif
