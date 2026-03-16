#ifndef MS_RTC_SCTP_ANY_DATA_CHUNK_HPP
#define MS_RTC_SCTP_ANY_DATA_CHUNK_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/UserData.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Base class for DataChunk and IDataChunk.
		 */

		class AnyDataChunk : public Chunk
		{
		protected:
			AnyDataChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
			{
			}

		public:
			/**
			 * The (I)mmediate bit (in DATA and I_DATA chunks).
			 */
			virtual bool GetI() const = 0;

			/**
			 * The (U)nordered bit (in DATA and I_DATA chunks).
			 */
			virtual bool GetU() const = 0;

			/**
			 * The (B)eginning fragment bit (in DATA and I_DATA chunks).
			 */
			virtual bool GetB() const = 0;

			/**
			 * The (E)nding fragment bit (in DATA and I_DATA chunks).
			 */
			virtual bool GetE() const = 0;

			/**
			 * TSN (in DATA and I_DATA chunks).
			 */
			virtual uint32_t GetTsn() const = 0;

			/**
			 * Stream Identifier (in DATA and I_DATA chunks).
			 */
			virtual uint16_t GetStreamId() const = 0;

			/**
			 * Stream Sequence Number (only in DATA chunks).
			 */
			virtual uint16_t GetStreamSequenceNumber() const = 0;

			/**
			 * Message Identifier (MID) (only in I_DATA chunks).
			 */
			virtual uint32_t GetMessageId() const = 0;

			/**
			 * Fragment Sequence Number (FSN) (only in I_DATA chunks).
			 */
			virtual uint32_t GetFragmentSequenceNumber() const = 0;

			/**
			 * Payload Protocol Identifier (PPID) (in DATA and I_DATA chunks).
			 */
			virtual uint32_t GetPayloadProtocolId() const = 0;

			virtual bool HasUserDataPayload() const = 0;

			virtual const uint8_t* GetUserDataPayload() const = 0;

			virtual uint16_t GetUserDataPayloadLength() const = 0;

			virtual UserData GetUserData() const = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
