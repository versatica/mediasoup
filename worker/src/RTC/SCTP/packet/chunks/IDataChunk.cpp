#define MS_CLASS "RTC::SCTP::IDataChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		IDataChunk* IDataChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			uint16_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::I_DATA)
			{
				MS_WARN_DEV("invalid Chunk type");

				return nullptr;
			}

			return IDataChunk::ParseStrict(buffer, bufferLength, chunkLength, padding);
		}

		IDataChunk* IDataChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < IDataChunk::IDataChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			auto* chunk = new IDataChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::I_DATA, 0, IDataChunk::IDataChunkHeaderLength);

			// Must also initialize extra fields in the header.
			chunk->SetTsn(0);
			chunk->SetStreamIdentifier(0);
			chunk->SetReserved();
			chunk->SetMessageIdentifier(0);
			chunk->SetPayloadProtocolIdentifierOrFragmentSequenceNumber(0);

			// No need to invoke SetLength() since constructor invoked it with
			// minimum IDataChunk length.

			return chunk;
		}

		IDataChunk* IDataChunk::ParseStrict(
		  const uint8_t* buffer, size_t bufferLength, uint16_t chunkLength, uint8_t padding)
		{
			MS_TRACE();

			if (chunkLength < IDataChunk::IDataChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "IDataChunk Length field must be equal or greater than %zu",
				  IDataChunk::IDataChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new IDataChunk(const_cast<uint8_t*>(buffer), bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		/* Instance methods. */

		IDataChunk::IDataChunk(uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(IDataChunk::IDataChunkHeaderLength);
		}

		IDataChunk::~IDataChunk()
		{
			MS_TRACE();
		}

		void IDataChunk::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::IDataChunk>");
			DumpCommon(indentation);
			MS_DUMP_CLEAN(indentation, "  flag I: %" PRIu8, GetI());
			MS_DUMP_CLEAN(indentation, "  flag U: %" PRIu8, GetU());
			MS_DUMP_CLEAN(indentation, "  flag B: %" PRIu8, GetB());
			MS_DUMP_CLEAN(indentation, "  flag E: %" PRIu8, GetE());
			MS_DUMP_CLEAN(indentation, "  tsn: %" PRIu32, GetTsn());
			MS_DUMP_CLEAN(indentation, "  stream identifier: %" PRIu16, GetStreamIdentifier());
			MS_DUMP_CLEAN(indentation, "  message identifier: %" PRIu32, GetMessageIdentifier());
			MS_DUMP_CLEAN(
			  indentation,
			  "  payload protocol identifier / fragment sequence number: %" PRIu32,
			  GetPayloadProtocolIdentifierOrFragmentSequenceNumber());
			MS_DUMP_CLEAN(
			  indentation,
			  "  user data length: %" PRIu16 " (has user data: %s)",
			  GetUserDataLength(),
			  HasUserData() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::IDataChunk>");
		}

		IDataChunk* IDataChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedChunk = new IDataChunk(buffer, bufferLength);

			CloneInto(clonedChunk);
			SoftCloneInto(clonedChunk);

			return clonedChunk;
		}

		void IDataChunk::SetI(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit3(flag);
		}

		void IDataChunk::SetU(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit2(flag);
		}

		void IDataChunk::SetB(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit1(flag);
		}

		void IDataChunk::SetE(bool flag)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetBit0(flag);
		}

		void IDataChunk::SetTsn(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void IDataChunk::SetStreamIdentifier(uint16_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		void IDataChunk::SetMessageIdentifier(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 12, value);
		}

		void IDataChunk::SetPayloadProtocolIdentifierOrFragmentSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			AssertNotFrozen();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 16, value);
		}

		void IDataChunk::SetUserData(const uint8_t* userData, uint16_t userDataLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetVariableLengthValue(userData, userDataLength);
		}

		IDataChunk* IDataChunk::SoftClone(const uint8_t* buffer) const
		{
			MS_TRACE();

			auto* softClonedChunk = new IDataChunk(const_cast<uint8_t*>(buffer), GetLength());

			SoftCloneInto(softClonedChunk);

			return softClonedChunk;
		}

		void IDataChunk::SetReserved()
		{
			MS_TRACE();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 10, 0);
		}
	} // namespace SCTP
} // namespace RTC
