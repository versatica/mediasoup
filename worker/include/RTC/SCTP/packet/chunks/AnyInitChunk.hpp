#ifndef MS_RTC_SCTP_ANY_INIT_CHUNK_HPP
#define MS_RTC_SCTP_ANY_INIT_CHUNK_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Base class for InitChunk and InitAckChunk.
		 */

		class AnyInitChunk : public Chunk
		{
		protected:
			AnyInitChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
			{
			}

		public:
			virtual uint32_t GetInitiateTag() const = 0;

			virtual uint32_t GetAdvertisedReceiverWindowCredit() const = 0;

			virtual uint16_t GetNumberOfOutboundStreams() const = 0;

			virtual uint16_t GetNumberOfInboundStreams() const = 0;

			virtual uint32_t GetInitialTsn() const = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
