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
		bool HasMuxId(const std::string& muxId, const RTC::Producer* producer) const;
		bool HasRid(const std::string& rid, const RTC::Producer* producer) const;
		void AddProducer(RTC::Producer* producer);
		void RemoveProducer(const RTC::Producer* producer);
		RTC::Producer* GetProducer(RTC::RtpPacket* packet);
		RTC::Producer* GetProducer(uint32_t ssrc);

	private:
		void RollbackProducer(
		  RTC::Producer* producer,
		  std::vector<uint32_t>& previousSsrcs,
		  std::string& previousMuxId,
		  std::string& previousRid);

	public:
		// Table of SSRC / Producer pairs.
		std::unordered_map<uint32_t, RTC::Producer*> ssrcTable;
		//  Table of MID RTP header extension / Producer pairs.
		std::unordered_map<std::string, RTC::Producer*> muxIdTable;
		//  Table of RID RTP header extension / Producer pairs.
		std::unordered_map<std::string, RTC::Producer*> ridTable;
	};

	/* Inline instance methods. */

	inline bool RtpListener::HasSsrc(uint32_t ssrc, const RTC::Producer* producer) const
	{
		auto it = this->ssrcTable.find(ssrc);

		if (it == this->ssrcTable.end())
			return false;
		else
			return (it->second != producer);
	}

	inline bool RtpListener::HasMuxId(const std::string& muxId, const RTC::Producer* producer) const
	{
		auto it = this->muxIdTable.find(muxId);

		if (it == this->muxIdTable.end())
			return false;
		else
			return (it->second != producer);
	}

	inline bool RtpListener::HasRid(const std::string& rid, const RTC::Producer* producer) const
	{
		auto it = this->ridTable.find(rid);

		if (it == this->ridTable.end())
			return false;
		else
			return (it->second != producer);
	}
} // namespace RTC

#endif
