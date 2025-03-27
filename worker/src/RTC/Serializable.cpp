#define MS_CLASS "RTC::Serializable"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Serializable.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Utils.hpp"

namespace RTC
{
	Serializable::Serializable(const uint8_t* buffer, size_t size)
	{
		MS_TRACE();

		if (buffer)
		{
			this->buffer = const_cast<uint8_t*>(buffer);
			this->size   = size;
		}
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
			MS_THROW_SERIALIZATION_ERROR("serialization needed");
		}

		return this->buffer;
	}

	uint8_t Serializable::GetPadding() const
	{
		MS_TRACE();

		return this->padding;
	}

	void Serializable::PadTo4Bytes()
	{
		MS_TRACE();

		auto previousSize = GetSize();
		auto newSize = Utils::Byte::PadTo4Bytes(static_cast<uint16_t>(previousSize - this->padding));
		auto padding = static_cast<uint8_t>(this->padding + newSize - previousSize);

		if (padding == this->padding)
		{
			return;
		}

		this->padding = padding;

		SetSerializationNeeded(true);
	}

	bool Serializable::NeedsSerialization() const
	{
		MS_TRACE();

		return this->serializationNeeded;
	}

	void Serializable::SetSerializationNeeded(bool flag)
	{
		MS_TRACE();

		this->serializationNeeded = flag;
	}
} // namespace RTC
