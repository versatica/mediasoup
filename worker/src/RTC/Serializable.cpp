#define MS_CLASS "RTC::Serializable"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Serializable.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <cstring> // std::memmove(), std::memset()

namespace RTC
{
	Serializable::Serializable(const uint8_t* buffer, size_t bufferLength)
	  : buffer(const_cast<uint8_t*>(buffer)), bufferLength(bufferLength)
	{
		MS_TRACE();
	}

	Serializable::~Serializable()
	{
		MS_TRACE();

		if (this->bufferReleasedListener)
		{
			(*this->bufferReleasedListener)(this, this->buffer);
		}
	}

	void Serializable::Serialize(uint8_t* buffer, size_t bufferLength)
	{
		MS_TRACE();

		// An item being built in place is beyond the current length, so it would
		// not be serialized.
		AssertDoesNotNeedConsolidation();

		if (bufferLength < this->length)
		{
			MS_THROW_TYPE_ERROR(
			  "bufferLength (%zu bytes) is lower than current length (%zu bytes)",
			  bufferLength,
			  this->length);
		}

		std::memmove(buffer, this->buffer, this->length);

		if (buffer != this->buffer && this->bufferReleasedListener)
		{
			(*this->bufferReleasedListener)(this, this->buffer);
		}

		this->buffer       = buffer;
		this->bufferLength = bufferLength;
	}

	void Serializable::SetBufferReleasedListener(Serializable::BufferReleasedListener* listener)
	{
		MS_TRACE();

		this->bufferReleasedListener = listener;
	}

	void Serializable::Consolidate() const
	{
		MS_TRACE();

		if (!this->buildInPlaceListener)
		{
			MS_THROW_ERROR("build in place listener not set");
		}

		AssertDoesNotNeedConsolidation();

		const BuildInPlaceListener listener = std::move(this->buildInPlaceListener);

		this->buildInPlaceListener = nullptr;

		listener(/*consolidate*/ true);
	}

	void Serializable::Discard() const
	{
		MS_TRACE();

		if (!this->buildInPlaceListener)
		{
			MS_THROW_ERROR("build in place listener not set");
		}

		// The listener of an item being built in place within this Serializable
		// refers to it, so it cannot be deleted.
		AssertDoesNotNeedConsolidation();

		// The listener deletes this Serializable, so it must be moved out of it
		// before being invoked.
		const BuildInPlaceListener listener = std::move(this->buildInPlaceListener);

		this->buildInPlaceListener = nullptr;

		listener(/*consolidate*/ false);

		// NOTE: This Serializable no longer exists here.
	}

	void Serializable::SetBuffer(uint8_t* buffer)
	{
		MS_TRACE();

		if (buffer == this->buffer)
		{
			return;
		}

		if (this->bufferReleasedListener)
		{
			(*this->bufferReleasedListener)(this, this->buffer);
		}

		this->buffer = buffer;
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

		if (bufferLength == 0)
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

		this->length = length;
	}

	void Serializable::CloneInto(Serializable* serializable) const
	{
		MS_TRACE();

		// An item being built in place is beyond the current length, so it would
		// not be cloned.
		if (this->needsConsolidation)
		{
			delete serializable;

			MS_THROW_ERROR("Serializable needs consolidation of some item being built in place");
		}

		if (serializable->GetBufferLength() < this->length)
		{
			const auto bufferLength = serializable->GetBufferLength();

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

		if (padding > this->length)
		{
			MS_THROW_TYPE_ERROR(
			  "padding (%" PRIu8 " bytes) cannot be greater than length (%zu bytes)", padding, this->length);
		}

		std::memset(this->buffer + this->length - padding, 0x00, padding);
	}

	void Serializable::SetBuildInPlaceListener(BuildInPlaceListener&& listener)
	{
		MS_TRACE();

		this->buildInPlaceListener = std::move(listener);
	}

	void Serializable::SetNeedsConsolidation(bool flag)
	{
		MS_TRACE();

		this->needsConsolidation = flag;
	}

	void Serializable::AssertDoesNotNeedConsolidation() const
	{
		MS_TRACE();

		if (this->needsConsolidation)
		{
			MS_THROW_ERROR("Serializable needs consolidation of some item being built in place");
		}
	}
} // namespace RTC
