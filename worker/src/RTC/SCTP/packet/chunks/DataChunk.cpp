#define MS_CLASS "RTC::SCTP::DataChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		DataChunk* DataChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::DATA)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return DataChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		DataChunk* DataChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < DataChunk::DataChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new DataChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::DATA, 0, DataChunk::DataChunkHeaderLength);

			// Must also initialize extra fields in the header.
			chunk->SetTsn(0);
			chunk->SetStreamIdentifierS(0);
			chunk->SetStreamSequenceNumberN(0);
			chunk->SetPayloadProtocolIdentifier(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum DataChunk length.

			return chunk;
		}

		DataChunk* DataChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength < DataChunk::DataChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "DataChunk Length field must be equal or greater than %zu",
				  DataChunk::DataChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new DataChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		DataChunk::DataChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(DataChunk::DataChunkHeaderLength);
		}

		DataChunk::~DataChunk()
		{
			MS_TRACE();
		}

		void DataChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::DataChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  flag I: %" PRIu8, GetI());
			MS_DUMP_CLEAN(indentation, "  flag U: %" PRIu8, GetU());
			MS_DUMP_CLEAN(indentation, "  flag B: %" PRIu8, GetB());
			MS_DUMP_CLEAN(indentation, "  flag E: %" PRIu8, GetE());
			MS_DUMP_CLEAN(indentation, "  tsn: %" PRIu32, GetTsn());
			MS_DUMP_CLEAN(indentation, "  stream identifier S: %" PRIu16, GetStreamIdentifierS());
			MS_DUMP_CLEAN(indentation, "  stream sequence number n: %" PRIu16, GetStreamSequenceNumberN());
			MS_DUMP_CLEAN(
			  indentation, "  payload protocol identifier: %" PRIu32, GetPayloadProtocolIdentifier());
			MS_DUMP_CLEAN(
			  indentation,
			  "  user data length: %" PRIu16 " (has user data: %s)",
			  GetUserDataLength(),
			  HasUserData() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::DataChunk>");
		}

		DataChunk* DataChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new DataChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void DataChunk::SetI(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit3(flag);
		}

		void DataChunk::SetU(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit2(flag);
		}

		void DataChunk::SetB(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit1(flag);
		}

		void DataChunk::SetE(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit0(flag);
		}

		void DataChunk::SetTsn(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void DataChunk::SetStreamIdentifierS(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		void DataChunk::SetStreamSequenceNumberN(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 10, value);
		}

		void DataChunk::SetPayloadProtocolIdentifier(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 12, value);
		}

		void DataChunk::SetUserData(const uint8_t* userData, uint16_t userDataLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(userData, userDataLength);
		}

		DataChunk* DataChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new DataChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}
	} // namespace SCTP
} // namespace RTC
