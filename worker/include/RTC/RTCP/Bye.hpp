#ifndef MS_RTC_RTCP_BYE_HPP
#define MS_RTC_RTCP_BYE_HPP

#include "common.hpp"
#include "RTC/RTCP/Packet.hpp"
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
		virtual ~ByePacket() {};

		void AddSsrc(uint32_t ssrc);
		void SetReason(const std::string& reason);
		const std::string& GetReason() const;
		Iterator Begin();
		Iterator End();

	/* Pure virtual methods inherited from Packet. */
	public:
		virtual void Dump() const override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetCount() const override;
		virtual size_t GetSize() const override;

	private:
		std::vector<uint32_t> ssrcs;
		std::string reason;
	};

	/* Inline instance methods. */

	inline
	ByePacket::ByePacket()
		: Packet(Type::BYE)
	{}

	inline
	size_t ByePacket::GetCount() const
	{
		return this->ssrcs.size();
	}

	inline
	size_t ByePacket::GetSize() const
	{
		size_t size = sizeof(Packet::CommonHeader);

		size += ssrcs.size() * sizeof(uint32_t);

		if (!this->reason.empty())
		{
			size += sizeof(uint8_t); // Length field.
			size += this->reason.length();
		}

		// http://stackoverflow.com/questions/11642210/computing-padding-required-for-n-byte-alignment
		// Consider pading to 32 bits (4 bytes) boundary.
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
	const std::string& ByePacket::GetReason() const
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
}}

#endif
