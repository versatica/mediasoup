#define MS_CLASS "RTC::Serializable"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Serializable.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"
#include <cstring> // std::memset()

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

	size_t Serializable::GetSize() const
	{
		MS_TRACE();

		if (this->serializationNeeded)
		{
			MS_THROW_ERROR("serialization needed");
		}

		return this->contentSize + this->padding;
	}

	bool Serializable::NeedsSerialization() const
	{
		MS_TRACE();

		return this->serializationNeeded;
	}

	void Serializable::SetInitialContentSize(size_t contentSize)
	{
		MS_TRACE();

		if (this->contentSize > 0u)
		{
			MS_THROW_ERROR("content size already initialized");
		}

		this->contentSize = contentSize;
	}

	void Serializable::SetInitialPadding(uint8_t padding)
	{
		MS_TRACE();

		if (this->padding > 0u)
		{
			MS_THROW_ERROR("padding already initialized");
		}

		this->padding = padding;
	}

	const uint8_t* Serializable::GetCurrentBuffer() const
	{
		MS_TRACE();

		return this->buffer;
	}

	size_t Serializable::GetCurrentContentSize() const
	{
		MS_TRACE();

		return this->contentSize;
	}

	uint8_t Serializable::GetCurrentPadding() const
	{
		MS_TRACE();

		return this->padding;
	}

	void Serializable::SetSerializationNeeded()
	{
		MS_TRACE();

		this->serializationNeeded = true;
	}

	void Serializable::Serialized(uint8_t* buffer, size_t contentSize, uint8_t padding)
	{
		MS_TRACE();

		this->buffer              = const_cast<uint8_t*>(buffer);
		this->contentSize         = contentSize;
		this->padding             = padding;
		this->serializationNeeded = false;

		// Fill padding bytes with zeroes.
		std::memset(this->buffer + this->contentSize, 0x00, this->padding);
	}
} // namespace RTC
