#ifndef MS_RTC_RTP_LISTENER_HPP
#define MS_RTC_RTP_LISTENER_HPP

#include "common.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RtpPacket.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

using json = nlohmann::json;

namespace RTC
{
	class RtpListener
	{
	public:
		void FillJson(json& jsonObject) const;
		void AddProducer(RTC::Producer* producer);
		void RemoveProducer(RTC::Producer* producer);
		RTC::Producer* GetProducer(const RTC::RtpPacket* packet);
		RTC::Producer* GetProducer(uint32_t ssrc) const;

	public:
		// Table of SSRC / Producer pairs.
		std::unordered_map<uint32_t, RTC::Producer*> ssrcTable;
		//  Table of MID / Producer pairs.
		std::unordered_map<std::string, RTC::Producer*> midTable;
		//  Table of RID / Producer pairs.
		std::unordered_map<std::string, RTC::Producer*> ridTable;
	};
} // namespace RTC

#endif
