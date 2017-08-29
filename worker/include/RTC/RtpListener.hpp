#ifndef MS_RTC_LISTENER_HPP
#define MS_RTC_LISTENER_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"
#include <string>
#include <unordered_map>
#include <vector>

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class Producer;

	class RtpListener
	{
	public:
		Json::Value ToJson() const;
		bool HasSsrc(uint32_t ssrc, const RTC::Producer* producer) const;
		bool HasMuxId(std::string& muxId, const RTC::Producer* producer) const;
		bool HasPayloadType(uint8_t payloadType, const RTC::Producer* producer) const;
		void AddProducer(RTC::Producer* producer);
		void RemoveProducer(const RTC::Producer* producer);
		RTC::Producer* GetProducer(RTC::RtpPacket* packet);
		RTC::Producer* GetProducer(uint32_t ssrc);

	private:
		void RollbackProducer(
		    RTC::Producer* producer,
		    std::vector<uint32_t>& previousSsrcs,
		    std::string& previousMuxId,
		    std::vector<uint8_t>& previousPayloadTypes);

	public:
		// Table of SSRC / Producer pairs.
		std::unordered_map<uint32_t, RTC::Producer*> ssrcTable;
		//  Table of MID RTP header extension / Producer pairs.
		std::unordered_map<std::string, const RTC::Producer*> muxIdTable;
		// Table of RTP payload type / Producer pairs.
		std::unordered_map<uint8_t, RTC::Producer*> ptTable;
	};

	/* Inline instance methods. */

	inline bool RtpListener::HasSsrc(uint32_t ssrc, const RTC::Producer* producer) const
	{
		auto it = this->ssrcTable.find(ssrc);

		if (it == this->ssrcTable.end())
		{
			return false;
		}
		else
		{
			return (it->second != producer);
		}
	}

	inline bool RtpListener::HasMuxId(std::string& muxId, const RTC::Producer* producer) const
	{
		auto it = this->muxIdTable.find(muxId);

		if (it == this->muxIdTable.end())
		{
			return false;
		}
		else
		{
			return (it->second != producer);
		}
	}

	inline bool RtpListener::HasPayloadType(uint8_t payloadType, const RTC::Producer* producer) const
	{
		auto it = this->ptTable.find(payloadType);

		if (it == this->ptTable.end())
		{
			return false;
		}
		else
		{
			return (it->second != producer);
		}
	}
} // namespace RTC

#endif
