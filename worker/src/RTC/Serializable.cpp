#define MS_CLASS "RTC::Serializable"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Serializable.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove(), std::memset()
#include <utility> // std::move()

namespace RTC
{
	void Serializable::Serialize(uint8_t* buffer, size_t bufferLength)
	{
		MS_TRACE();

		if (bufferLength < this->length)
		{
			MS_THROW_TYPE_ERROR(
			  "bufferLength (%zu bytes) is lower than current length (%zu bytes)",
			  bufferLength,
			  this->length);
		}

		std::memmove(buffer, this->buffer, this->length);

		this->buffer       = buffer;
		this->bufferLength = bufferLength;
		this->frozen       = false;
	}

	void Serializable::Consolidate()
	{
		MS_TRACE();

		AssertNotFrozen();

		if (!this->consolidatedListener)
		{
			MS_THROW_ERROR("consolidated listener not set");
		}

		this->consolidatedListener();
	}

	void Serializable::SetBufferLength(size_t bufferLength)
	{
		MS_TRACE();

		AssertNotFrozen();

		if (bufferLength < this->length)
		{
			MS_THROW_TYPE_ERROR(
			  "buffer length (%zu bytes) is lower than current length (%zu bytes)",
			  bufferLength,
			  this->length);
		}

		if (bufferLength == 0)
		{
			MS_THROW_TYPE_ERROR("bufferLength cannot be 0");
		}

		this->bufferLength = bufferLength;
	}

	void Serializable::SetLength(size_t length)
	{
		MS_TRACE();

		AssertNotFrozen();

		if (length > this->bufferLength)
		{
			MS_THROW_TYPE_ERROR(
			  "length (%zu bytes) is larger than internal buffer maximum length (%zu bytes)",
			  length,
			  this->bufferLength);
		}

		if (length == 0)
		{
			MS_THROW_TYPE_ERROR("length cannot be 0");
		}

		this->length = length;
	}

	void Serializable::CloneInto(Serializable* serializable) const
	{
		MS_TRACE();

		if (serializable->GetBufferLength() < this->length)
		{
			delete serializable;

			MS_THROW_TYPE_ERROR(
			  "bufferLength (%zu bytes) is lower than current length (%zu bytes)",
			  bufferLength,
			  this->length);
		}

		std::memmove(const_cast<uint8_t*>(serializable->GetBuffer()), this->buffer, this->length);

		// Need to manually set Serializable length.
		serializable->SetLength(this->length);
	}

	void Serializable::FillPadding(uint8_t padding)
	{
		MS_TRACE();

		AssertNotFrozen();

		if (padding > this->length)
		{
			MS_THROW_TYPE_ERROR(
			  "padding (%" PRIu8 " bytes) cannot be greater than length (%zu bytes)", padding, this->length);
		}

		std::memset(this->buffer + this->length - padding, 0x00, padding);
	}

	void Serializable::SetConsolidatedListener(const ConsolidatedListener&& listener)
	{
		MS_TRACE();

		AssertNotFrozen();

		this->consolidatedListener = std::move(listener);
	}

	void Serializable::AssertNotFrozen() const
	{
		MS_TRACE();

		if (this->frozen)
		{
			MS_THROW_ERROR("Serializable is frozen");
		}
	}
} // namespace RTC
