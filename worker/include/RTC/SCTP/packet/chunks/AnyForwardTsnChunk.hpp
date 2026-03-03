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
				  : streamId(streamId), ssn(ssn), unordered(false), mid(0)
				{
				}

				SkippedStream(uint16_t streamId, bool unordered, uint32_t mid)
				  : streamId(streamId), ssn(0), unordered(unordered), mid(mid)
				{
				}

				uint16_t streamId;

				/**
				 * Only set for FORWARD_TSN.
				 */
				uint16_t ssn;

				/**
				 * Only set for I_FORWARD_TSN.
				 */
				bool unordered;

				/**
				 * Only set for I_FORWARD_TSN.
				 */
				uint32_t mid;

				bool operator==(const SkippedStream& other) const
				{
					return streamId == other.streamId && ssn == other.ssn && unordered == other.unordered &&
					       mid == other.mid;
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
			return os << "{streamId:" << s.streamId << ", ssn:" << s.ssn << ", unordered:" << s.unordered
			          << ", mid:" << s.mid << "}";
		}
	} // namespace SCTP
} // namespace RTC

#endif
