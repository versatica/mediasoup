#define MS_CLASS "RTC::RTCPPacket"

#include "RTC/RTCPPacket.h"
#include "Logger.h"


namespace RTC {


/* Static methods. */

RTCPPacket* RTCPPacket::Parse(const MS_BYTE* data, size_t len) {
	MS_TRACE();

	if (! RTCPPacket::IsRTCP(data, len))
		return nullptr;

	// TODO: Do it.

	// Get the header.
	CommonHeader* header = (CommonHeader*)data;

	return new RTCPPacket(header, data, len);
}


/* Instance methods. */

RTCPPacket::RTCPPacket(CommonHeader* header, const MS_BYTE* raw, size_t length) :
	header(header),
	raw((MS_BYTE*)raw),
	length(length)
{
	MS_TRACE();
}


RTCPPacket::~RTCPPacket() {
	MS_TRACE();
}


}  // namespace RTC
