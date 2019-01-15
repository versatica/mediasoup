#define MS_CLASS "RTC::RtpListener"
// #define MS_LOG_DEV

#include "RTC/RtpListener.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "RTC/Producer.hpp"

namespace RTC
{
	/* Instance methods. */

	void RtpListener::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		jsonObject["ssrcTable"] = json::object();
		jsonObject["midTable"]  = json::object();
		jsonObject["ridTable"]  = json::object();

		auto jsonSsrcTableIt = jsonObject.find("ssrcTable");
		auto jsonMidTableIt  = jsonObject.find("midTable");
		auto jsonRidTableIt  = jsonObject.find("ridTable");

		// Add ssrcTable.
		for (auto& kv : this->ssrcTable)
		{
			auto ssrc      = kv.first;
			auto* producer = kv.second;

			(*jsonSsrcTableIt)[std::to_string(ssrc)] = producer->producerId;
		}

		// Add midTable.
		for (auto& kv : this->midTable)
		{
			auto mid       = kv.first;
			auto* producer = kv.second;

			(*jsonMidTableIt)[mid] = producer->producerId;
		}

		// Add ridTable.
		for (auto& kv : this->ridTable)
		{
			auto rid       = kv.first;
			auto* producer = kv.second;

			jsonRidTable[rid]      = std::to_string(producer->producerId);
			(*jsonRidTableIt)[mid] = producer->producerId;
		}

		return json;
	}

	void RtpListener::AddProducer(RTC::Producer* producer)
	{
		MS_TRACE();

		auto& rtpParameters = producer->GetRtpParameters();

		// Keep a copy of the previous entries so we can rollback.

		std::vector<uint32_t> previousSsrcs;
		std::string previousMid;
		std::vector<std::string> previousRids;

		for (auto& kv : this->ssrcTable)
		{
			auto ssrc              = kv.first;
			auto& existingProducer = kv.second;

			if (existingProducer == producer)
				previousSsrcs.push_back(ssrc);
		}

		for (auto& kv : this->midTable)
		{
			auto& mid              = kv.first;
			auto& existingProducer = kv.second;

			if (existingProducer == producer)
			{
				previousMid = mid;

				break;
			}
		}

		for (auto& kv : this->ridTable)
		{
			auto& rid              = kv.first;
			auto& existingProducer = kv.second;

			if (existingProducer == producer)
				previousRids.push_back(rid);
		}

		// First remove from the listener tables all the entries pointing to
		// the given Producer.
		RemoveProducer(producer);

		// Add entries into the ssrcTable.
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
					RollbackProducer(producer, previousSsrcs, previousMid, previousRids);

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
					RollbackProducer(producer, previousSsrcs, previousMid, previousRids);

					MS_THROW_ERROR("ssrc already exists in RTP listener [ssrc:%" PRIu32 "]", ssrc);
				}
			}
		}

		// Add entries into midTable.
		if (!rtpParameters.mid.empty())
		{
			auto& mid = rtpParameters.mid;

			if (!HasMid(mid, producer))
			{
				this->midTable[mid] = producer;
			}
			else
			{
				RemoveProducer(producer);
				RollbackProducer(producer, previousSsrcs, previousMid, previousRids);

				MS_THROW_ERROR("mid already exists in RTP listener [mid:'%s']", mid.c_str());
			}
		}

		// Add entries into ridTable.
		for (auto& encoding : rtpParameters.encodings)
		{
			auto& rid = encoding.rid;

			if (rid.empty())
				continue;

			if (!HasRid(rid, producer))
			{
				this->ridTable[rid] = producer;
			}
			else
			{
				RemoveProducer(producer);
				RollbackProducer(producer, previousSsrcs, previousMid, previousRids);

				MS_THROW_ERROR("rid already exists in RTP listener [rid:'%s']", rid.c_str());
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

		for (auto it = this->midTable.begin(); it != this->midTable.end();)
		{
			if (it->second == producer)
				it = this->midTable.erase(it);
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
				auto* producer = it->second;

				return producer;
			}
		}

		// Otherwise lookup into the MID table.
		{
			const uint8_t* midPtr;
			size_t midLen;

			if (packet->ReadMid(&midPtr, &midLen))
			{
				auto* charMidPtr = reinterpret_cast<const char*>(midPtr);
				std::string mid(charMidPtr, midLen);

				auto it = this->midTable.find(mid);
				if (it != this->midTable.end())
				{
					auto* producer = it->second;

					// Fill the ssrc table.
					this->ssrcTable[packet->GetSsrc()] = producer;

					return producer;
				}
			}
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
					auto* producer = it->second;

					// Fill the ssrc table.
					this->ssrcTable[packet->GetSsrc()] = producer;

					return producer;
				}
			}
		}

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
	  const std::vector<std::string>& previousMids,
	  const std::vector<std::string>& previousRids)
	{
		MS_TRACE();

		for (auto ssrc : previousSsrcs)
		{
			this->ssrcTable[ssrc] = producer;
		}

		if (!previousMid.empty())
			this->midTable[previousMid] = producer;

		for (auto& rid : previousRids)
		{
			this->ridTable[prid] = producer;
		}
	}
} // namespace RTC
