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
		static const Json::StaticString JsonStringRidTable{ "ridTable" };

		Json::Value json(Json::objectValue);
		Json::Value jsonSsrcTable(Json::objectValue);
		Json::Value jsonMuxIdTable(Json::objectValue);
		Json::Value jsonRidTable(Json::objectValue);

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

		// Add ridTable.
		for (auto& kv : this->ridTable)
		{
			auto rid      = kv.first;
			auto producer = kv.second;

			jsonRidTable[rid] = std::to_string(producer->producerId);
		}
		json[JsonStringRidTable] = jsonRidTable;

		return json;
	}

	void RtpListener::AddProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		auto& rtpParameters = producer->GetParameters();

		// Keep a copy of the previous entries so we can rollback.

		std::vector<uint32_t> previousSsrcs;
		std::string previousMuxId;
		std::string previousRid;

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

		for (auto& kv : this->ridTable)
		{
			auto& rid              = kv.first;
			auto& existingProducer = kv.second;

			if (existingProducer == producer)
			{
				previousRid = rid;

				break;
			}
		}

		// First remove from the listener tables all the entries pointing to
		// the given Producer.
		RemoveProducer(producer);

		// Add entries into the ssrcTable.
		{
			for (auto& encoding : rtpParameters.encodings)
			{
				uint32_t ssrc;

				// Check encoding.ssrc.

				ssrc = encoding.ssrc;

				if (ssrc != 0u)
				{
					if (!HasSsrc(ssrc, producer))
					{
						this->ssrcTable[ssrc] = producer;
					}
					else
					{
						RemoveProducer(producer);
						RollbackProducer(producer, previousSsrcs, previousMuxId, previousRid);

						MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
					}
				}

				// Check encoding.rtx.ssrc.

				ssrc = encoding.rtx.ssrc;

				if (ssrc != 0u)
				{
					if (!HasSsrc(ssrc, producer))
					{
						this->ssrcTable[ssrc] = producer;
					}
					else
					{
						RemoveProducer(producer);
						RollbackProducer(producer, previousSsrcs, previousMuxId, previousRid);

						MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
					}
				}

				// Check encoding.fec.ssrc.

				ssrc = encoding.fec.ssrc;

				if (ssrc != 0u)
				{
					if (!HasSsrc(ssrc, producer))
					{
						this->ssrcTable[ssrc] = producer;
					}
					else
					{
						RemoveProducer(producer);
						RollbackProducer(producer, previousSsrcs, previousMuxId, previousRid);

						MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
					}
				}
			}
		}

		// Add entries into muxIdTable.
		{
			if (!rtpParameters.muxId.empty())
			{
				auto& muxId = rtpParameters.muxId;

				if (!HasMuxId(muxId, producer))
				{
					this->muxIdTable[muxId] = producer;
				}
				else
				{
					RemoveProducer(producer);
					RollbackProducer(producer, previousSsrcs, previousMuxId, previousRid);

					MS_THROW_ERROR("muxId already exists in RTP listener [muxId:'%s']", muxId.c_str());
				}
			}
		}

		// Add entries into ridTable.
		{
			for (auto& encoding : rtpParameters.encodings)
			{
				auto& rid = encoding.encodingId;

				if (rid.empty())
					continue;

				if (!HasRid(rid, producer))
				{
					this->ridTable[rid] = producer;
				}
				else
				{
					RemoveProducer(producer);
					RollbackProducer(producer, previousSsrcs, previousMuxId, previousRid);

					MS_THROW_ERROR("rid already exists in RTP listener [rid:'%s']", rid.c_str());
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

		for (auto it = this->ridTable.begin(); it != this->ridTable.end();)
		{
			if (it->second == producer)
				it = this->ridTable.erase(it);
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
				auto producer = it->second;

				return producer;
			}
		}

		// Otherwise lookup into the muxId table.
		{
			// TODO: https://github.com/versatica/mediasoup/issues/230
			// const uint8_t* muxIdPtr;
			// size_t muxIdLen;

			// if (packet->ReadMid(&muxIdPtr, &muxIdLen))
			// {
			// 	auto* charMuxIdPtr = reinterpret_cast<const char*>(muxIdPtr);
			// 	std::string muxId(charMuxIdPtr, muxIdLen);

			// 	auto it = this->muxIdTable.find(muxId);
			// 	if (it != this->muxIdTable.end())
			// 	{
			// 		auto producer = it->second;

			// 		// Fill the ssrc table.
			// 		this->ssrcTable[packet->GetSsrc()] = producer;

			// 		return producer;
			// 	}
			// }
		}

		// Otherwise lookup into the RID table.
		{
			const uint8_t* ridPtr;
			size_t ridLen;

			if (packet->ReadRid(&ridPtr, &ridLen))
			{
				auto* charRidPtr = reinterpret_cast<const char*>(ridPtr);
				std::string rid(charRidPtr, ridLen);

				auto it = this->ridTable.find(rid);
				if (it != this->ridTable.end())
				{
					auto producer = it->second;

					// Fill the ssrc table.
					this->ssrcTable[packet->GetSsrc()] = producer;

					return producer;
				}
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
			return it->second;

		return nullptr;
	}

	void RtpListener::RollbackProducer(
	  RTC::Producer* producer,
	  const std::vector<uint32_t>& previousSsrcs,
	  const std::string& previousMuxId,
	  const std::string& previousRid)
	{
		MS_TRACE();

		for (auto ssrc : previousSsrcs)
		{
			this->ssrcTable[ssrc] = producer;
		}

		if (!previousMuxId.empty())
			this->muxIdTable[previousMuxId] = producer;

		if (!previousRid.empty())
			this->ridTable[previousRid] = producer;
	}
} // namespace RTC
