#define MS_CLASS "RTC::RtpListener"
// #define MS_LOG_DEV

#include "RTC/RtpListener.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/Producer.hpp"

namespace RTC
{
	/* Instance methods. */

	Json::Value RtpListener::ToJson() const
	{
		MS_TRACE();

		static const Json::StaticString JsonStringSsrcTable{ "ssrcTable" };
		static const Json::StaticString JsonStringMuxIdTable{ "muxIdTable" };
		static const Json::StaticString JsonStringPtTable{ "ptTable" };

		Json::Value json(Json::objectValue);
		Json::Value jsonSsrcTable(Json::objectValue);
		Json::Value jsonMuxIdTable(Json::objectValue);
		Json::Value jsonPtTable(Json::objectValue);

		// Add ssrcTable.
		for (auto& kv : this->ssrcTable)
		{
			auto ssrc     = kv.first;
			auto producer = kv.second;

			jsonSsrcTable[std::to_string(ssrc)] = std::to_string(producer->producerId);
		}
		json[JsonStringSsrcTable] = jsonSsrcTable;

		// Add muxIdTable.
		for (auto& kv : this->muxIdTable)
		{
			auto muxId    = kv.first;
			auto producer = kv.second;

			jsonMuxIdTable[muxId] = std::to_string(producer->producerId);
		}
		json[JsonStringMuxIdTable] = jsonMuxIdTable;

		// Add ptTable.
		for (auto& kv : this->ptTable)
		{
			auto payloadType = kv.first;
			auto producer    = kv.second;

			jsonPtTable[std::to_string(payloadType)] = std::to_string(producer->producerId);
		}
		json[JsonStringPtTable] = jsonPtTable;

		return json;
	}

	void RtpListener::AddProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		auto rtpParameters = producer->GetParameters();

		MS_ASSERT(rtpParameters, "no RtpParameters");

		// Keep a copy of the previous entries so we can rollback.

		std::vector<uint32_t> previousSsrcs;
		std::string previousMuxId;
		std::vector<uint8_t> previousPayloadTypes;

		for (auto& kv : this->ssrcTable)
		{
			auto ssrc              = kv.first;
			auto& existingProducer = kv.second;

			if (existingProducer == producer)
				previousSsrcs.push_back(ssrc);
		}

		for (auto& kv : this->muxIdTable)
		{
			auto& muxId            = kv.first;
			auto& existingProducer = kv.second;

			if (existingProducer == producer)
			{
				previousMuxId = muxId;
				break;
			}
		}

		for (auto& kv : this->ptTable)
		{
			auto payloadType       = kv.first;
			auto& existingProducer = kv.second;

			if (existingProducer == producer)
				previousPayloadTypes.push_back(payloadType);
		}

		// First remove from the listener tables all the entries pointing to
		// the given Producer.
		RemoveProducer(producer);

		// Add entries into the ssrcTable.
		{
			for (auto& encoding : rtpParameters->encodings)
			{
				uint32_t ssrc;

				// Check encoding.ssrc.

				ssrc = encoding.ssrc;

				if (ssrc != 0u)
				{
					if (!this->HasSsrc(ssrc, producer))
					{
						this->ssrcTable[ssrc] = producer;
					}
					else
					{
						RemoveProducer(producer);
						RollbackProducer(producer, previousSsrcs, previousMuxId, previousPayloadTypes);

						MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
					}
				}

				// Check encoding.rtx.ssrc.

				ssrc = encoding.rtx.ssrc;

				if (ssrc != 0u)
				{
					if (!this->HasSsrc(ssrc, producer))
					{
						this->ssrcTable[ssrc] = producer;
					}
					else
					{
						RemoveProducer(producer);
						RollbackProducer(producer, previousSsrcs, previousMuxId, previousPayloadTypes);

						MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
					}
				}

				// Check encoding.fec.ssrc.

				ssrc = encoding.fec.ssrc;

				if (ssrc != 0u)
				{
					if (!this->HasSsrc(ssrc, producer))
					{
						this->ssrcTable[ssrc] = producer;
					}
					else
					{
						RemoveProducer(producer);
						RollbackProducer(producer, previousSsrcs, previousMuxId, previousPayloadTypes);

						MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
					}
				}
			}
		}

		// Add entries into muxIdTable.
		{
			if (!rtpParameters->muxId.empty())
			{
				auto& muxId = rtpParameters->muxId;

				if (!this->HasMuxId(muxId, producer))
				{
					this->muxIdTable[muxId] = producer;
				}
				else
				{
					RemoveProducer(producer);
					RollbackProducer(producer, previousSsrcs, previousMuxId, previousPayloadTypes);

					MS_THROW_ERROR("muxId already exists in RTP listener [muxId:'%s']", muxId.c_str());
				}
			}
		}

		// Add entries into ptTable just if:
		// - Not all the encoding.ssrc are given, or
		// - Not all the encoding.rtx.ssrc are given, or
		// - Not all the encoding.fec.ssrc are given.
		{
			auto it = rtpParameters->encodings.begin();

			for (; it != rtpParameters->encodings.end(); ++it)
			{
				auto& encoding = *it;

				if ((encoding.ssrc == 0u) || (encoding.hasRtx && (encoding.rtx.ssrc == 0u)) ||
				    (encoding.hasFec && (encoding.fec.ssrc == 0u)))
				{
					break;
				}
			}

			if (it != rtpParameters->encodings.end())
			{
				for (auto& codec : rtpParameters->codecs)
				{
					uint8_t payloadType = codec.payloadType;

					if (!this->HasPayloadType(payloadType, producer))
					{
						this->ptTable[payloadType] = producer;
					}
					else
					{
						RemoveProducer(producer);
						RollbackProducer(producer, previousSsrcs, previousMuxId, previousPayloadTypes);

						MS_THROW_ERROR(
						    "payloadType already exists in RTP listener [payloadType:%" PRIu8 "]", payloadType);
					}
				}
			}
		}
	}

	void RtpListener::RemoveProducer(const RTC::Producer* producer)
	{
		MS_TRACE();

		// Remove from the listener tables all the entries pointing to the given
		// Producer.

		for (auto it = this->ssrcTable.begin(); it != this->ssrcTable.end();)
		{
			if (it->second == producer)
				it = this->ssrcTable.erase(it);
			else
				++it;
		}

		for (auto it = this->muxIdTable.begin(); it != this->muxIdTable.end();)
		{
			if (it->second == producer)
				it = this->muxIdTable.erase(it);
			else
				++it;
		}

		for (auto it = this->ptTable.begin(); it != this->ptTable.end();)
		{
			if (it->second == producer)
				it = this->ptTable.erase(it);
			else
				++it;
		}
	}

	RTC::Producer* RtpListener::GetProducer(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// First lookup into the SSRC table.
		{
			auto it = this->ssrcTable.find(packet->GetSsrc());

			if (it != this->ssrcTable.end())
			{
				auto producer      = it->second;
				auto rtpParameters = producer->GetParameters();

				// Ensure the RTP PT is present in RtpParameters.
				for (auto& codec : rtpParameters->codecs)
				{
					// Check payloads.
					if (codec.payloadType == packet->GetPayloadType())
						return producer;
				}

				// RTP PT not present.
				MS_WARN_TAG(rtp, "unknown RTP payloadType [payloadType:%" PRIu8 "]", packet->GetPayloadType());

				// TODO: We may emit "unhandledrtp" event.
				return nullptr;
			}
		}

		// TODO: RID table? Sure.

		// Otherwise lookup into the muxId table.
		// TODO: do it.

		// Otherwise lookup into the PT table.
		{
			auto it = this->ptTable.find(packet->GetPayloadType());

			if (it != this->ptTable.end())
			{
				auto producer = it->second;

				// Update SSRC table.
				this->ssrcTable[packet->GetSsrc()] = producer;

				return producer;
			}
		}

		// TODO: We may emit "unhandledrtp" event.
		return nullptr;
	}

	RTC::Producer* RtpListener::GetProducer(uint32_t ssrc)
	{
		MS_TRACE();

		// Lookup into the SSRC table.
		auto it = this->ssrcTable.find(ssrc);

		if (it != this->ssrcTable.end())
		{
			return it->second;
		}

		return nullptr;
	}

	void RtpListener::RollbackProducer(
	    RTC::Producer* producer,
	    std::vector<uint32_t>& previousSsrcs,
	    std::string& previousMuxId,
	    std::vector<uint8_t>& previousPayloadTypes)
	{
		MS_TRACE();

		for (auto ssrc : previousSsrcs)
		{
			this->ssrcTable[ssrc] = producer;
		}

		if (!previousMuxId.empty())
			this->muxIdTable[previousMuxId] = producer;

		for (auto payloadType : previousPayloadTypes)
		{
			this->ptTable[payloadType] = producer;
		}
	}
} // namespace RTC
