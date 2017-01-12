#define MS_CLASS "RTC::RtpSender"
// #define MS_LOG_DEV

#include "RTC/RtpSender.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <unordered_set>

namespace RTC
{
	/* Instance methods. */

	RtpSender::RtpSender(Listener* listener, Channel::Notifier* notifier, uint32_t rtpSenderId, RTC::Media::Kind kind) :
		rtpSenderId(rtpSenderId),
		kind(kind),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();
	}

	RtpSender::~RtpSender()
	{
		MS_TRACE();
	}

	void RtpSender::Close()
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");

		if (this->rtpParameters)
			delete this->rtpParameters;

		Json::Value event_data(Json::objectValue);

		event_data[k_class] = "RtpSender";
		this->notifier->Emit(this->rtpSenderId, "close", event_data);

		// Notify the listener.
		this->listener->onRtpSenderClosed(this);

		delete this;
	}

	Json::Value RtpSender::toJson()
	{
		MS_TRACE();

		static Json::Value null_data(Json::nullValue);
		static const Json::StaticString k_rtpSenderId("rtpSenderId");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_hasTransport("hasTransport");
		static const Json::StaticString k_available("available");
		static const Json::StaticString k_mapPayloadTypes("mapPayloadTypes");

		Json::Value json(Json::objectValue);

		json[k_rtpSenderId] = (Json::UInt)this->rtpSenderId;

		json[k_kind] = RTC::Media::GetJsonString(this->kind);

		if (this->rtpParameters)
			json[k_rtpParameters] = this->rtpParameters->toJson();
		else
			json[k_rtpParameters] = null_data;

		json[k_hasTransport] = this->transport ? true : false;

		json[k_available] = this->available;

		json[k_mapPayloadTypes] = Json::objectValue;
		for (auto& kv : this->mapPayloadTypes)
		{
			json[k_mapPayloadTypes][std::to_string(kv.first)] = kv.second;
		}

		return json;
	}

	void RtpSender::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::rtpSender_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void RtpSender::SetPeerCapabilities(RTC::RtpCapabilities* peerCapabilities)
	{
		MS_TRACE();

		MS_ASSERT(peerCapabilities, "given peerCapabilities are null");

		this->peerCapabilities = peerCapabilities;
	}

	void RtpSender::Send(RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_available("available");

		MS_ASSERT(rtpParameters, "no RTP parameters given");

		auto previousRtpParameters = this->rtpParameters;

		// Free the previous rtpParameters.
		if (previousRtpParameters)
			delete this->rtpParameters;

		// Clone given RTP parameters so we manage our own sender parameters.
		this->rtpParameters = new RTC::RtpParameters(rtpParameters);

		// Remove RTP parameters not supported by this Peer.

		// Remove unsupported codecs.
		std::unordered_set<uint8_t> supportedPayloadTypes;

		for (auto it = this->rtpParameters->codecs.begin(); it != this->rtpParameters->codecs.end();)
		{
			auto& codec = *it;
			auto it2 = this->peerCapabilities->codecs.begin();

			for (; it2 != this->peerCapabilities->codecs.end(); it2++)
			{
				auto& codecCapability = *it2;

				if (codecCapability.Matches(codec))
					break;
			}

			if (it2 != this->peerCapabilities->codecs.end())
			{
				supportedPayloadTypes.insert(codec.payloadType);
				it++;
			}
			else
			{
				it = this->rtpParameters->codecs.erase(it);
			}
		}

		// Remove unsupported encodings.
		for (auto it = this->rtpParameters->encodings.begin(); it != this->rtpParameters->encodings.end();)
		{
			auto& encoding = *it;

			if (supportedPayloadTypes.find(encoding.codecPayloadType) != supportedPayloadTypes.end())
			{
				it++;
			}
			else
			{
				it = this->rtpParameters->encodings.erase(it);
			}
		}

		// TODO: Remove unsupported header extensions.

		// Build the payload types map.
		SetPayloadTypesMapping();

		// If there are no encodings set not available.
		if (this->rtpParameters->encodings.size() > 0)
			this->available = true;
		else
			this->available = false;

		// Emit "parameterschange" if these are updated parameters.
		if (previousRtpParameters)
		{
			Json::Value event_data(Json::objectValue);

			event_data[k_class] = "RtpSender";
			event_data[k_rtpParameters] = this->rtpParameters->toJson();
			event_data[k_available] = this->available;

			this->notifier->Emit(this->rtpSenderId, "parameterschange", event_data);
		}
	}

	void RtpSender::SendRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!this->available || !this->transport)
			return;

		// Map the payload type.

		uint8_t originalPayloadType = packet->GetPayloadType();
		uint8_t mappedPayloadType;
		auto it = this->mapPayloadTypes.find(originalPayloadType);

		// NOTE: This may happen if this peer supports just some codecs from the
		// given RtpParameters.
		// TODO: We should not report an error here but just ignore it.
		if (it == this->mapPayloadTypes.end())
		{
			MS_ERROR("payload type not mapped [payloadType:%" PRIu8 "]", originalPayloadType);

			return;
		}
		mappedPayloadType = this->mapPayloadTypes[originalPayloadType];

		// Map the packet payload type.
		packet->SetPayloadType(mappedPayloadType);

		// Send the packet.
		this->transport->SendRtpPacket(packet);

		// Recover the original packet payload type.
		packet->SetPayloadType(originalPayloadType);
	}

	void RtpSender::RetransmitRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		if (!this->available || !this->transport)
			return;

		// If the peer supports RTX create a RTX packet and insert the given media
		// packet as payload. Otherwise just send the packet as usual.
		// TODO: No RTX for now so just send as usual.
		SendRtpPacket(packet);
	}

	void RtpSender::SetPayloadTypesMapping()
	{
		MS_TRACE();

		MS_ASSERT(this->peerCapabilities, "peer RTP capabilities are null");

		this->mapPayloadTypes.clear();

		for (auto& codec : this->rtpParameters->codecs)
		{
			auto it = this->peerCapabilities->codecs.begin();

			for (; it != this->peerCapabilities->codecs.end(); it++)
			{
				auto& codecCapability = *it;

				if (codecCapability.Matches(codec))
				{
					auto originalPayloadType = codec.payloadType;
					auto mappedPayloadType = codecCapability.payloadType;

					// Set the mapping.
					this->mapPayloadTypes[originalPayloadType] = mappedPayloadType;

					// Override the codec payload type.
					codec.payloadType = mappedPayloadType;

					// Override encoding.codecPayloadType.
					for (auto& encoding : this->rtpParameters->encodings)
					{
						// TODO: remove when confirmed
						MS_ASSERT(encoding.hasCodecPayloadType, "encoding without codecPayloadType");

						if (encoding.codecPayloadType == originalPayloadType)
							encoding.codecPayloadType = mappedPayloadType;
					}

					break;
				}
			}

			// This should not happen.
			MS_ASSERT(it != this->peerCapabilities->codecs.end(),
				"no matching room codec found [payloadType:%" PRIu8 "]", codec.payloadType);
		}
	}
}
