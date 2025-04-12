#define MS_CLASS "RTC::SCTP::DataChunk"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/chunks/DataChunk.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove()

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		DataChunk* DataChunk::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			Chunk::ChunkType chunkType;
			size_t chunkLength;
			uint8_t padding;

			if (!Chunk::IsChunk(buffer, bufferLength, chunkType, chunkLength, padding))
			{
				return nullptr;
			}

			if (chunkType != Chunk::ChunkType::DATA)
			{
				MS_WARN_DEV("invalid chunk type");

				return nullptr;
			}

			// UserData cannot have 0 length so we use <= rather than <.
			if (chunkLength <= DataChunk::DataChunkHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "DataChunk Length field must have value greater than %zu",
				  DataChunk::DataChunkHeaderLength);

				return nullptr;
			}

			auto* chunk = new DataChunk(buffer, bufferLength);

			// Must always invoke SetLength() after constructing a Serializable with
			// not fixed length.
			chunk->SetLength(chunkLength + padding);

			// Mark the Chunk as frozen since we are parsing.
			chunk->Freeze();

			return chunk;
		}

		DataChunk* DataChunk::Factory(uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength < DataChunk::DataChunkHeaderLength)
			{
				MS_THROW_TYPE_ERROR("too small buffer");
			}

			auto* chunk = new DataChunk(buffer, bufferLength);

			chunk->InitializeHeader(Chunk::ChunkType::DATA, 0, DataChunk::DataChunkHeaderLength);
			// Must also initialize extra fields in the header.
			chunk->InitializeExtraHeader();

			// No need to invoke SetLength() since constructor invoked it with
			// minimum DataChunk length.

			return chunk;
		}

		/* Instance methods. */

		DataChunk::DataChunk(const uint8_t* buffer, size_t bufferLength) : Chunk(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(DataChunk::DataChunkHeaderLength);
		}

		DataChunk::~DataChunk()
		{
			MS_TRACE();
		}

		void DataChunk::Dump() const
		{
			MS_TRACE();

			MS_DUMP("<DataChunk>");
			MS_DUMP("  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP(
			  "  type: %" PRIu8 " (%s) (unknown:%s)",
			  static_cast<uint8_t>(GetType()),
			  Chunk::ChunkType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP("  flag I: %" PRIu8, GetI());
			MS_DUMP("  flag U: %" PRIu8, GetU());
			MS_DUMP("  flag B: %" PRIu8, GetB());
			MS_DUMP("  flag E: %" PRIu8, GetE());
			MS_DUMP("  tsn: %" PRIu32, GetTSN());
			MS_DUMP("  stream identifier S: %" PRIu16, GetStreamIdentifierS());
			MS_DUMP("  stream sequence number n: %" PRIu16, GetStreamSequenceNumberN());
			MS_DUMP("  payload protocol identifier: %" PRIu32, GetPayloadProtocolIdentifier());
			MS_DUMP("  user data length: %zu", GetUserDataLength());
			MS_DUMP("</DataChunk>");
		}

		DataChunk* DataChunk::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedItem = new DataChunk(buffer, bufferLength);

			CloneInto(clonedItem);

			return clonedItem;
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

		void DataChunk::SetTSN(uint32_t value)
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

		void DataChunk::SetUserData(const uint8_t* userData, size_t userDataLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousUserDataLength = GetUserDataLength();
			auto newNotPaddedLength     = GetLength() - previousUserDataLength + userDataLength;
			auto newPaddedLength        = Utils::Byte::PadTo4Bytes(newNotPaddedLength);

			// Let's call SetLength() on parent with the new computed chunk length.
			// NOTE: If there is no space in the buffer for it, it will throw.
			// NOTE: Chunks must be padded to 4 bytes.
			SetLength(newPaddedLength);

			// Copy the given user data into the buffer.
			std::memmove(GetUserDataPointer(), userData, userDataLength);

			// Update the Chunk Length field.
			SetLengthField(newNotPaddedLength);

			// Fill padding bytes with zero.
			FillPadding(newPaddedLength - newNotPaddedLength);
		}

		void DataChunk::InitializeExtraHeader()
		{
			MS_TRACE();

			SetTSN(0);
			SetStreamIdentifierS(0);
			SetStreamSequenceNumberN(0);
			SetPayloadProtocolIdentifier(0);
		}
	} // namespace SCTP
} // namespace RTC
