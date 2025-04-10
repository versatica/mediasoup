#define MS_CLASS "RTC::Serializable"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Serializable.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

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

		Utils::Buffer::MemcpyOrMemmove(buffer, this->buffer, this->length);

		this->buffer       = buffer;
		this->bufferLength = bufferLength;

		// May unfreeze the Serializable.
		Unfreeze();
	}

	void Serializable::SetBufferLength(size_t bufferLength)
	{
		MS_TRACE();

		if (bufferLength < this->length)
		{
			MS_THROW_TYPE_ERROR(
			  "buffer length (%zu bytes) is lower than current length (%zu bytes)",
			  bufferLength,
			  this->length);
		}

		if (bufferLength == 0u)
		{
			MS_THROW_TYPE_ERROR("bufferLength cannot be 0");
		}

		this->bufferLength = bufferLength;
	}

	void Serializable::SetLength(size_t length)
	{
		MS_TRACE();

		if (length > this->bufferLength)
		{
			MS_THROW_TYPE_ERROR(
			  "length (%zu bytes) is larger than internal buffer maximum length (%zu bytes)",
			  length,
			  this->bufferLength);
		}

		if (length == 0u)
		{
			MS_THROW_TYPE_ERROR("length cannot be 0");
		}

		this->length = length;
	}

	void Serializable::CloneInto(Serializable* serializable) const
	{
		MS_TRACE();

		if (serializable->GetBufferLength() < GetLength())
		{
			MS_THROW_TYPE_ERROR(
			  "bufferLength (%zu bytes) is lower than current length (%zu bytes)",
			  bufferLength,
			  GetLength());
		}

		Utils::Buffer::MemcpyOrMemmove(
		  const_cast<uint8_t*>(serializable->GetBuffer()), GetBuffer(), GetLength());

		// Need to manually set Serializable length.
		serializable->SetLength(GetLength());
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
