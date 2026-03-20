#define MS_CLASS "RTC::SCTP::IDataChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"

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
			chunk->SetStreamId(0);
			chunk->SetReserved();
			chunk->SetMessageId(0);
			// NOTE: BitB is not set so we must set FSN to 0 rather than setting PPID.
			chunk->SetFragmentSequenceNumber(0);

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

			return chunk;
		}

		/* Instance methods. */

		IDataChunk::IDataChunk(uint8_t* buffer, size_t bufferLength)
		  : AnyDataChunk(buffer, bufferLength)
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
			MS_DUMP_CLEAN(indentation, "  stream id: %" PRIu16, GetStreamId());
			MS_DUMP_CLEAN(indentation, "  message id: %" PRIu32, GetMessageId());
			if (GetB())
			{
				MS_DUMP_CLEAN(indentation, "  payload protocol id (PPID): %" PRIu32, GetPayloadProtocolId());
			}
			else
			{
				MS_DUMP_CLEAN(
				  indentation, "  fragment sequence number (FSN): %" PRIu32, GetFragmentSequenceNumber());
			}
			MS_DUMP_CLEAN(
			  indentation,
			  "  user data length: %" PRIu16 " (has user data: %s)",
			  GetUserDataPayloadLength(),
			  HasUserDataPayload() ? "yes" : "no");
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

			SetBit3(flag);
		}

		void IDataChunk::SetU(bool flag)
		{
			MS_TRACE();

			SetBit2(flag);
		}

		void IDataChunk::SetB(bool flag)
		{
			MS_TRACE();

			SetBit1(flag);
		}

		void IDataChunk::SetE(bool flag)
		{
			MS_TRACE();

			SetBit0(flag);
		}

		void IDataChunk::SetTsn(uint32_t value)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 4, value);
		}

		void IDataChunk::SetStreamId(uint16_t value)
		{
			MS_TRACE();

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 8, value);
		}

		void IDataChunk::SetMessageId(uint32_t value)
		{
			MS_TRACE();

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 12, value);
		}

		void IDataChunk::SetPayloadProtocolId(uint32_t value)
		{
			MS_TRACE();

			if (!GetB())
			{
				MS_THROW_ERROR("cannot set payload protocol id (PPID) if bit B is not set");
			}

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 16, value);
		}

		void IDataChunk::SetFragmentSequenceNumber(uint32_t value)
		{
			MS_TRACE();

			if (GetB())
			{
				MS_THROW_ERROR("cannot set payload protocol id (PPID) if bit B is set");
			}

			Utils::Byte::Set4Bytes(const_cast<uint8_t*>(GetBuffer()), 16, value);
		}

		void IDataChunk::SetUserDataPayload(const uint8_t* userDataPayload, uint16_t userDataPayloadLength)
		{
			MS_TRACE();

			SetVariableLengthValue(userDataPayload, userDataPayloadLength);
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
