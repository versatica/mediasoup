#define MS_CLASS "RTC::Serializable"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Serializable.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	Serializable::Serializable(uint8_t* buffer) : buffer(buffer)
	{
		MS_TRACE();
	}

	Serializable::~Serializable()
	{
		MS_TRACE();
	}

	const uint8_t* Serializable::GetBuffer() const
	{
		MS_TRACE();

		if (this->serializationNeeded)
		{
			MS_THROW_ERROR("serialization needed");
		}

		return this->buffer;
	}

	bool Serializable::NeedsSerialization() const
	{
		MS_TRACE();

		return this->serializationNeeded;
	}

	void Serializable::Parsed(size_t size)
	{
		MS_TRACE();

		if (this->size)
		{
			MS_THROW_ERROR("Parsed() already called");
		}

		this->size                = size;
		this->serializationNeeded = false;
	}

	const uint8_t* Serializable::GetCurrentBuffer() const
	{
		MS_TRACE();

		return this->buffer;
	}

	size_t Serializable::GetCurrentSize() const
	{
		MS_TRACE();

		return this->size;
	}

	void Serializable::SetSerializationNeeded()
	{
		MS_TRACE();

		this->serializationNeeded = true;
	}

	void Serializable::Serialized(uint8_t* buffer, size_t size)
	{
		MS_TRACE();

		this->buffer              = const_cast<uint8_t*>(buffer);
		this->size                = size;
		this->serializationNeeded = false;
	}
} // namespace RTC
