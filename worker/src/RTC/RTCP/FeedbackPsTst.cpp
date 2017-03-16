#define MS_CLASS "RTC::RTCP::FeedbackPsTst"
// #define MS_LOG_DEV

#include "RTC/RTCP/FeedbackPsTst.hpp"
#include "Logger.hpp"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	template <typename T>
	FeedbackPsTstItem<T>* FeedbackPsTstItem<T>::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// data size must be >= header + length value.
		if (sizeof(Header) > len)
		{
			MS_WARN_TAG(rtcp, "not enough space for Tst item, discarded");

			return nullptr;
		}

		Header* header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

		return new FeedbackPsTstItem(header);
	}

	template <typename T>
	FeedbackPsTstItem<T>::FeedbackPsTstItem(uint32_t ssrc, uint8_t sequenceNumber, uint8_t index)
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
	size_t FeedbackPsTstItem<T>::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		std::memcpy(buffer, this->header, sizeof(Header));

		return sizeof(Header);
	}

	template <typename T>
	void FeedbackPsTstItem<T>::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<FeedbackPsTstItem>");
		MS_DUMP("  ssrc            : %" PRIu32, this->GetSsrc());
		MS_DUMP("  sequence number : %" PRIu32, this->GetSequenceNumber());
		MS_DUMP("  index           : %" PRIu32, this->GetIndex());
		MS_DUMP("</FeedbackPsTstItem>");
	}

	/* Specialization for Tstr class. */

	template<>
	const FeedbackPs::MessageType FeedbackPsTstItem<Tstr>::MessageType = FeedbackPs::TSTR;

	/* Specialization for Tstn class. */

	template<>
	const FeedbackPs::MessageType FeedbackPsTstItem<Tstn>::MessageType = FeedbackPs::TSTN;

	// Explicit instantiation to have all definitions in this file.
	template class FeedbackPsTstItem<Tstr>;
	template class FeedbackPsTstItem<Tstn>;
}}
