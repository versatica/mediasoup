#define MS_CLASS "RTC::Serializable"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/Serializable.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memcpy()

namespace RTC
{
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

		this->bufferLength = bufferLength;
	}

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

		std::memcpy(buffer, this->buffer, this->length);

		this->buffer       = buffer;
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

		this->length = length;
	}
} // namespace RTC
