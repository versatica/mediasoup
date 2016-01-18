#define MS_CLASS "RTC::RtpListener"

#include "RTC/RtpListener.h"
#include "Logger.h"
#include <string>

namespace RTC
{
	/* Instance methods. */

	RtpListener::RtpListener()
	{
		MS_TRACE();
	}

	RtpListener::~RtpListener()
	{
		MS_TRACE();
	}

	Json::Value RtpListener::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_ssrcTable("ssrcTable");
		// static const Json::StaticString k_midTable("midTable");  // TODO
		static const Json::StaticString k_ptTable("ptTable");

		Json::Value json(Json::objectValue);

		// Add `ssrcTable`.
		Json::Value json_ssrcTable(Json::objectValue);

		for (auto& kv : this->ssrcTable)
		{
			auto ssrc = kv.first;
			auto rtpReceiver = kv.second;

			json_ssrcTable[std::to_string(ssrc)] = std::to_string(rtpReceiver->rtpReceiverId);
		}
		json[k_ssrcTable] = json_ssrcTable;

		// TODO: Add `midTable`.

		// Add `ptTable`.
		Json::Value json_ptTable(Json::objectValue);

		for (auto& kv : this->ptTable)
		{
			auto payloadType = kv.first;
			auto rtpReceiver = kv.second;

			json_ptTable[std::to_string(payloadType)] = std::to_string(rtpReceiver->rtpReceiverId);
		}
		json[k_ptTable] = json_ptTable;

		return json;
	}

	void RtpListener::SetRtpReceiver(RTC::RtpReceiver* rtpReceiver, RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		// First remove from the rtpListener all the entries pointing to the given rtpReceiver.
		// TODO
		RemoveRtpReceiver(rtpReceiver);

		// Add entries into ssrcTable.
		for (auto& encoding : rtpParameters->encodings)
		{
			// TODO: This should throw if the ssrc already exists in the table,
			// and the receive() should Reject() the Request.
			if (encoding.ssrc)
				this->ssrcTable[encoding.ssrc] = rtpReceiver;
		}

		// TODO: // Add entries into midTable.

		// Add entries into ptTable.
		// TODO: This is wrong, pt should not be added if ssrc are added.
		for (auto& codec : rtpParameters->codecs)
		{
			this->ptTable[codec.payloadType] = rtpReceiver;
		}
	}

	void RtpListener::RemoveRtpReceiver(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// Remove from the RtpListener all the entries pointing to the given RtpReceiver.

		for (auto it = this->ssrcTable.begin(); it != this->ssrcTable.end();)
		{
			if (it->second == rtpReceiver)
				it = this->ssrcTable.erase(it);
			else
				it++;
		}

		// TODO: midTable

		for (auto it = this->ptTable.begin(); it != this->ptTable.end();)
		{
			if (it->second == rtpReceiver)
				it = this->ptTable.erase(it);
			else
				it++;
		}
	}
}
