#ifndef MS_RTC_RTCP_BYE_H
#define MS_RTC_RTCP_BYE_H

#include "common.h"
#include "RTC/RTCP/Packet.h"

#include <vector>
#include <string>

namespace RTC { namespace RTCP
{

	class ByePacket
		: public Packet
	{
	public:
		typedef std::vector<uint32_t>::iterator Iterator;

	public:
		static ByePacket* Parse(const uint8_t* data, size_t len);

	public:
		ByePacket();

		// Virtual methods inherited from Packet
		void Dump() override;
		size_t Serialize(uint8_t* data) override;
		size_t GetCount() override;
		size_t GetSize() override;

	public:
		void AddSsrc(uint32_t ssrc);
		void SetReason(const std::string& reason);
		const std::string& GetReason();
		Iterator Begin();
		Iterator End();

	private:
		std::vector<uint32_t> ssrcs;
		std::string reason;
	};

	/* BYE Packet inline instance methods */

	inline
	ByePacket::ByePacket()
		: Packet(Type::BYE)
	{
	}

	inline
	size_t ByePacket::GetCount()
	{
		return this->ssrcs.size();
	}

	inline
	size_t ByePacket::GetSize()
	{
		size_t size = sizeof(Packet::CommonHeader);
		size += ssrcs.size() * sizeof(uint32_t);

		if (!this->reason.empty())
		{
			size += sizeof(uint8_t);     // length field
			size += this->reason.length();
		}

		// http://stackoverflow.com/questions/11642210/computing-padding-required-for-n-byte-alignment
		// Consider pading to 32 bits (4 bytes) boundary
		return (size + 3) & ~3;
	}

	inline
	void ByePacket::AddSsrc(uint32_t ssrc)
	{
		this->ssrcs.push_back(ssrc);
	}

	inline
	void ByePacket::SetReason(const std::string& reason)
	{
		this->reason = reason;
	}

	inline
	const std::string& ByePacket::GetReason()
	{
		return this->reason;
	}

	inline
	ByePacket::Iterator ByePacket::Begin()
	{
		return this->ssrcs.begin();
	}

	inline
	ByePacket::Iterator ByePacket::End()
	{
		return this->ssrcs.end();
	}

} } // RTP::RTCP

#endif
