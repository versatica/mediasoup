#define MS_CLASS "RTC::RtpListener"
// #define MS_LOG_DEV

#include "RTC/RtpListener.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"

namespace RTC
{
	/* Instance methods. */

	Json::Value RtpListener::toJson() const
	{
		MS_TRACE();

		static const Json::StaticString k_ssrcTable("ssrcTable");
		static const Json::StaticString k_muxIdTable("muxIdTable");
		static const Json::StaticString k_ptTable("ptTable");

		Json::Value json(Json::objectValue);
		Json::Value jsonSsrcTable(Json::objectValue);
		Json::Value jsonMuxIdTable(Json::objectValue);
		Json::Value jsonPtTable(Json::objectValue);

		// Add `ssrcTable`.
		for (auto& kv : this->ssrcTable)
		{
			auto ssrc        = kv.first;
			auto rtpReceiver = kv.second;

			jsonSsrcTable[std::to_string(ssrc)] = std::to_string(rtpReceiver->rtpReceiverId);
		}
		json[k_ssrcTable] = jsonSsrcTable;

		// Add `muxIdTable`.
		for (auto& kv : this->muxIdTable)
		{
			auto muxId       = kv.first;
			auto rtpReceiver = kv.second;

			jsonMuxIdTable[muxId] = std::to_string(rtpReceiver->rtpReceiverId);
		}
		json[k_muxIdTable] = jsonMuxIdTable;

		// Add `ptTable`.
		for (auto& kv : this->ptTable)
		{
			auto payloadType = kv.first;
			auto rtpReceiver = kv.second;

			jsonPtTable[std::to_string(payloadType)] = std::to_string(rtpReceiver->rtpReceiverId);
		}
		json[k_ptTable] = jsonPtTable;

		return json;
	}

	void RtpListener::AddRtpReceiver(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		auto rtpParameters = rtpReceiver->GetParameters();

		MS_ASSERT(rtpParameters, "no RtpParameters");

		// Keep a copy of the previous entries so we can rollback.

		std::vector<uint32_t> previousSsrcs;
		std::string previousMuxId;
		std::vector<uint8_t> previousPayloadTypes;

		for (auto& kv : this->ssrcTable)
		{
			auto ssrc                 = kv.first;
			auto& existingRtpReceiver = kv.second;

			if (existingRtpReceiver == rtpReceiver)
				previousSsrcs.push_back(ssrc);
		}

		for (auto& kv : this->muxIdTable)
		{
			auto& muxId               = kv.first;
			auto& existingRtpReceiver = kv.second;

			if (existingRtpReceiver == rtpReceiver)
			{
				previousMuxId = muxId;
				break;
			}
		}

		for (auto& kv : this->ptTable)
		{
			auto payloadType          = kv.first;
			auto& existingRtpReceiver = kv.second;

			if (existingRtpReceiver == rtpReceiver)
				previousPayloadTypes.push_back(payloadType);
		}

		// First remove from the listener tables all the entries pointing to
		// the given RtpReceiver.
		RemoveRtpReceiver(rtpReceiver);

		// Add entries into the ssrcTable.
		{
			for (auto& encoding : rtpParameters->encodings)
			{
				uint32_t ssrc;

				// Check encoding.ssrc.

				ssrc = encoding.ssrc;

				if (ssrc)
				{
					if (!this->HasSsrc(ssrc, rtpReceiver))
					{
						this->ssrcTable[ssrc] = rtpReceiver;
					}
					else
					{
						RemoveRtpReceiver(rtpReceiver);
						RollbackRtpReceiver(rtpReceiver, previousSsrcs, previousMuxId, previousPayloadTypes);

						MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
					}
				}

				// Check encoding.rtx.ssrc.

				ssrc = encoding.rtx.ssrc;

				if (ssrc)
				{
					if (!this->HasSsrc(ssrc, rtpReceiver))
					{
						this->ssrcTable[ssrc] = rtpReceiver;
					}
					else
					{
						RemoveRtpReceiver(rtpReceiver);
						RollbackRtpReceiver(rtpReceiver, previousSsrcs, previousMuxId, previousPayloadTypes);

						MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
					}
				}

				// Check encoding.fec.ssrc.

				ssrc = encoding.fec.ssrc;

				if (ssrc)
				{
					if (!this->HasSsrc(ssrc, rtpReceiver))
					{
						this->ssrcTable[ssrc] = rtpReceiver;
					}
					else
					{
						RemoveRtpReceiver(rtpReceiver);
						RollbackRtpReceiver(rtpReceiver, previousSsrcs, previousMuxId, previousPayloadTypes);

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

				if (!this->HasMuxId(muxId, rtpReceiver))
				{
					this->muxIdTable[muxId] = rtpReceiver;
				}
				else
				{
					RemoveRtpReceiver(rtpReceiver);
					RollbackRtpReceiver(rtpReceiver, previousSsrcs, previousMuxId, previousPayloadTypes);

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

				if (!encoding.ssrc || (encoding.hasRtx && !encoding.rtx.ssrc) ||
				    (encoding.hasFec && !encoding.fec.ssrc))
				{
					break;
				}
			}

			if (it != rtpParameters->encodings.end())
			{
				for (auto& codec : rtpParameters->codecs)
				{
					uint8_t payloadType = codec.payloadType;

					if (!this->HasPayloadType(payloadType, rtpReceiver))
					{
						this->ptTable[payloadType] = rtpReceiver;
					}
					else
					{
						RemoveRtpReceiver(rtpReceiver);
						RollbackRtpReceiver(rtpReceiver, previousSsrcs, previousMuxId, previousPayloadTypes);

						MS_THROW_ERROR(
						    "payloadType already exists in RTP listener [payloadType:%" PRIu8 "]", payloadType);
					}
				}
			}
		}
	}

	void RtpListener::RemoveRtpReceiver(const RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// Remove from the listener tables all the entries pointing to the given
		// RtpReceiver.

		for (auto it = this->ssrcTable.begin(); it != this->ssrcTable.end();)
		{
			if (it->second == rtpReceiver)
				it = this->ssrcTable.erase(it);
			else
				++it;
		}

		for (auto it = this->muxIdTable.begin(); it != this->muxIdTable.end();)
		{
			if (it->second == rtpReceiver)
				it = this->muxIdTable.erase(it);
			else
				++it;
		}

		for (auto it = this->ptTable.begin(); it != this->ptTable.end();)
		{
			if (it->second == rtpReceiver)
				it = this->ptTable.erase(it);
			else
				++it;
		}
	}

	RTC::RtpReceiver* RtpListener::GetRtpReceiver(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// First lookup into the SSRC table.
		{
			auto it = this->ssrcTable.find(packet->GetSsrc());

			if (it != this->ssrcTable.end())
			{
				auto rtpReceiver   = it->second;
				auto rtpParameters = rtpReceiver->GetParameters();

				// Ensure the RTP PT is present in RtpParameters.
				for (auto& codec : rtpParameters->codecs)
				{
					// Check payloads.
					if (codec.payloadType == packet->GetPayloadType())
						return rtpReceiver;
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
				auto rtpReceiver = it->second;

				// Update SSRC table.
				this->ssrcTable[packet->GetSsrc()] = rtpReceiver;

				return rtpReceiver;
			}
		}

		// TODO: We may emit "unhandledrtp" event.
		return nullptr;
	}

	RTC::RtpReceiver* RtpListener::GetRtpReceiver(uint32_t ssrc)
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

	void RtpListener::RollbackRtpReceiver(
	    RTC::RtpReceiver* rtpReceiver,
	    std::vector<uint32_t>& previousSsrcs,
	    std::string& previousMuxId,
	    std::vector<uint8_t>& previousPayloadTypes)
	{
		MS_TRACE();

		for (auto ssrc : previousSsrcs)
		{
			this->ssrcTable[ssrc] = rtpReceiver;
		}

		if (!previousMuxId.empty())
			this->muxIdTable[previousMuxId] = rtpReceiver;

		for (auto payloadType : previousPayloadTypes)
		{
			this->ptTable[payloadType] = rtpReceiver;
		}
	}
}
