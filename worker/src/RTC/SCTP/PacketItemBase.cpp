#define MS_CLASS "RTC::SCTP::PacketItemBase"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/PacketItemBase.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove()
#include <limits>  // std::numeric_limits()

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		bool PacketItemBase::IsPacketItemBase(
		  const uint8_t* buffer, size_t bufferLength, uint16_t& itemLength, uint8_t& padding)
		{
			MS_TRACE();

			if (bufferLength < PacketItemBase::PacketItemBaseHeaderLength)
			{
				MS_WARN_TAG(sctp, "no space for Header [bufferLength:%zu]", bufferLength);

				return false;
			}

			itemLength = Utils::Byte::Get2Bytes(buffer, 2);

			if (itemLength < PacketItemBase::PacketItemBaseHeaderLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "Length field must have value greater or equal than %zu",
				  PacketItemBase::PacketItemBaseHeaderLength);

				return false;
			}

			// Item total length must be multiple of 4 bytes and must include padding
			// bytes despite item Length field doesn't not include padding.
			// NOTE: We must cast to size_t, otherwise a maximum item Length value of
			// 65535 would generate a padded length of 0 bytes!
			size_t paddedItemLength = Utils::Byte::PadTo4Bytes(size_t{ itemLength });

			if (bufferLength < paddedItemLength)
			{
				MS_WARN_TAG(
				  sctp,
				  "no space for 4-byte padded announced Length [paddedItemLength:%zu, bufferLength:%zu]",
				  paddedItemLength,
				  bufferLength);

				return false;
			}

			padding = paddedItemLength - itemLength;

			return true;
		}

		/* Instance methods. */

		PacketItemBase::PacketItemBase(uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		PacketItemBase::~PacketItemBase()
		{
			MS_TRACE();
		}

		void PacketItemBase::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation,
			  "  length field: %" PRIu16 " (padding: %zu, buffer length: %zu)",
			  GetLengthField(),
			  GetLength() - GetLengthField(),
			  GetBufferLength());
			MS_DUMP_CLEAN(indentation, "  frozen: %s", IsFrozen() ? "yes" : "no");
		}

		void PacketItemBase::InitializePacketBaseItemHeader(uint16_t lengthFieldValue)
		{
			MS_TRACE();

			AssertNotFrozen();

			SetLengthField(lengthFieldValue);
		}

		void PacketItemBase::SetLengthField(size_t lengthField)
		{
			MS_TRACE();

			if (lengthField > std::numeric_limits<uint16_t>::max())
			{
				MS_THROW_TYPE_ERROR("lengthField (%zu bytes) cannot be greater than 65535", lengthField);
			}

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 2, lengthField);
		}

		void PacketItemBase::SetVariableLengthValue(const uint8_t* value, size_t valueLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			// NOTE: This can throw.
			SetVariableLengthValueLength(valueLength);

			// Copy the given value into the buffer.
			std::memmove(GetVariableLengthValuePointer(), value, valueLength);
		}

		void PacketItemBase::SetVariableLengthValueLength(size_t valueLength)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousLength      = GetLength();
			auto previousLengthField = GetLengthField();
			auto previousValueLength = GetVariableLengthValueLength();
			auto newNotPaddedLength =
			  size_t{ previousLengthField } - size_t{ previousValueLength } + valueLength;
			auto newPaddedLength = Utils::Byte::PadTo4Bytes(newNotPaddedLength);

			try
			{
				// Let's call SetLength() on parent with the new computed length.
				// NOTE: If there is no space in the buffer for it, it will throw.
				SetLength(newPaddedLength);

				// Update Length field.
				// NOTE: This will throw if computed value is too big.
				SetLengthField(newNotPaddedLength);
			}
			catch (const MediaSoupError& error)
			{
				// Rollback.
				SetLength(previousLength);
				SetLengthField(previousLengthField);

				throw;
			}

			// Fill padding bytes with zero.
			FillPadding(newPaddedLength - newNotPaddedLength);
		}

		void PacketItemBase::AddItem(const PacketItemBase* item)
		{
			MS_TRACE();

			AssertNotFrozen();

			auto previousLength      = GetLength();
			auto previousLengthField = GetLengthField();

			try
			{
				// Update length.
				// NOTE: This will throw if there is no enough space in the buffer.
				SetLength(previousLength + item->GetLength());

				// Update Length field.
				// NOTE: This will throw if computed Length field value is too big.
				SetLengthField(previousLength + item->GetLengthField());
			}
			catch (const MediaSoupError& error)
			{
				// Rollback.
				SetLength(previousLength);
				SetLengthField(previousLengthField);

				throw;
			}
		}
	} // namespace SCTP
} // namespace RTC
