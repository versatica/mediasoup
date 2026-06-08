#ifndef MS_RTC_RTP_LISTENER_HPP
#define MS_RTC_RTP_LISTENER_HPP

#include "common.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTP/Packet.hpp"
#include <ankerl/unordered_dense.h>
#include <string>

namespace RTC
{
	class RtpListener
	{
	public:
		flatbuffers::Offset<FBS::Transport::RtpListener> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const;
		void AddProducer(RTC::Producer* producer);
		void RemoveProducer(RTC::Producer* producer);
		RTC::Producer* GetProducer(const RTC::RTP::Packet* packet);
		RTC::Producer* GetProducer(uint32_t ssrc) const;

	public:
		// Table of SSRC / Producer pairs.
		ankerl::unordered_dense::map<uint32_t, RTC::Producer*> ssrcTable;
		//  Table of MID / Producer pairs.
		ankerl::unordered_dense::map<std::string, RTC::Producer*> midTable;
		//  Table of RID / Producer pairs.
		ankerl::unordered_dense::map<std::string, RTC::Producer*> ridTable;
	};
} // namespace RTC

#endif
