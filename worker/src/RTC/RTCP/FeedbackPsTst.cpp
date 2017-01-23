#define MS_CLASS "RTC::RTCP::FeedbackPsTstPacket"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsTst.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	template <typename T>
	TstItem<T>* TstItem<T>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Tst item, discarded");

			return nullptr;
		}

		Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

		return new TstItem(header);
	}

	template <typename T>
	TstItem<T>::TstItem(uint32_t ssrc, uint8_t sequenceNumber, uint8_t index)
	{
		MS_TRACE();

		this->raw = new uint8_t[sizeof(Header)];
		this->header = reinterpret_cast<Header*>(this->raw);

		// Set reserved bits to zero.
		std::memset(this->header, 0, sizeof(Header));

		this->header->ssrc = htonl(ssrc);
		this->header->sequence_number = sequenceNumber;
		this->header->index = index;
	}

	template <typename T>
	size_t TstItem<T>::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		std::memcpy(buffer, this->header, sizeof(Header));

		return sizeof(Header);
	}

	template <typename T>
	void TstItem<T>::Dump()
	{
		MS_TRACE();

		MS_DEBUG_DEV("<TstItem>");
		MS_DEBUG_DEV("  ssrc            : %" PRIu32, (uint32_t)ntohl(this->header->ssrc));
		MS_DEBUG_DEV("  sequence number : %" PRIu32, this->header->sequence_number);
		MS_DEBUG_DEV("  index           : %" PRIu32, this->header->index);
		MS_DEBUG_DEV("</TstItem>");
	}

	/* Specialization for Tstr class. */

	template<>
	const FeedbackPs::MessageType TstItem<Tstr>::MessageType = FeedbackPs::TSTR;

	/* Specialization for Tstn class. */

	template<>
	const FeedbackPs::MessageType TstItem<Tstn>::MessageType = FeedbackPs::TSTN;

	// Explicit instantiation to have all definitions in this file.
	template class TstItem<Tstr>;
	template class TstItem<Tstn>;
}}
