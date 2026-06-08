#ifndef MS_RTC_SCTP_LISTENER_HPP
#define MS_RTC_SCTP_LISTENER_HPP

#include "common.hpp"
#include "RTC/DataProducer.hpp"
#include <ankerl/unordered_dense.h>

namespace RTC
{
	class SctpListener
	{
	public:
		flatbuffers::Offset<FBS::Transport::SctpListener> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const;
		void AddDataProducer(RTC::DataProducer* dataProducer);
		void RemoveDataProducer(RTC::DataProducer* dataProducer);
		RTC::DataProducer* GetDataProducer(uint16_t streamId);

	public:
		// Table of streamId / DataProducer pairs.
		ankerl::unordered_dense::map<uint16_t, RTC::DataProducer*> streamIdTable;
	};
} // namespace RTC

#endif
