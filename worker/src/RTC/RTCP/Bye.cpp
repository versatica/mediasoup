#define MS_CLASS "RTC::RTCP::Bye"

#include "RTC/RTCP/Bye.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>

namespace RTC { namespace RTCP
{
	/* Class methods. */

	ByePacket* ByePacket::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Get the header.
		Packet::CommonHeader* header = (Packet::CommonHeader*)data;

		std::auto_ptr<ByePacket> packet(new ByePacket());

		size_t offset = sizeof(Packet::CommonHeader);

		uint8_t count = header->count;
		while ((count--) && (len - offset > 0))
		{
			if (sizeof(uint32_t) > len-offset)
			{
				MS_WARN("not enough space for SSRC in RTCP Bye message");
				return nullptr;
			}

			packet->AddSsrc(ntohl(Utils::Byte::Get4Bytes(data, offset)));
			offset += sizeof(uint32_t);
		}

		if (len - offset > 0)
		{
			size_t length = size_t(Utils::Byte::Get1Byte(data, offset));
			offset += sizeof(uint8_t);
			if (length <= len-offset)
			{
				packet->SetReason(std::string((char*)data+offset, length));
			}
		}

		return packet.release();
	}

	/* Instance methods. */

	size_t ByePacket::Serialize(uint8_t* data)
	{
		MS_TRACE();

		size_t offset = Packet::Serialize(data);

		// SSRCs.
		for (auto ssrc : this->ssrcs)
		{
			Utils::Byte::Set4Bytes(data, offset, htonl(ssrc));
			offset += sizeof(uint32_t);
		}

		if (!this->reason.empty())
		{
			// Length field.
			Utils::Byte::Set1Byte(data, offset, this->reason.length());
			offset += sizeof(uint8_t);

			// Reason field.
			std::memcpy(data+offset, this->reason.c_str(), this->reason.length());
			offset += this->reason.length();
		}

		// 32 bits padding.
		size_t padding = (-offset) & 3;
		for (size_t i = 0; i < padding; i++)
		{
			data[offset+i] = 0;
		}

		return offset+padding;
	}

	void ByePacket::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		MS_WARN("<Bye Packet>");

		for (auto ssrc : this->ssrcs)
		{
			MS_WARN("\tssrc: %u", ssrc);
		}

		if (!this->reason.empty())
			MS_WARN("\treason: %s", this->reason.c_str());

		MS_WARN("</Bye Packet>");
	}
}}
