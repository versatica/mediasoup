#define MS_CLASS "RTC::RtpSender"

#include "RTC/RtpSender.h"
#include "MediaSoupError.h"
#include "Logger.h"

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
		// TODO: TMP
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

		// TODO: TMP
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

		// Build the payload types map.
		SetPayloadTypesMapping();

		// TODO: Must check new parameters and:
		// - remove unsuported capabilities,
		// - set this->available if at least there is a capable encoding.
		// For now make it easy:
		this->available = true;

		// Emit "parameterschange" if those are new parameters.
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

		// TODO: This should never happen.
		if (it == this->mapPayloadTypes.end())
		{
			MS_ERROR("payload type not mapped [payloadType:%" PRIu8 "]",
				originalPayloadType);

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

	void RtpSender::SetPayloadTypesMapping()
	{
		MS_TRACE();

		MS_ASSERT(this->peerCapabilities, "peer RTP capabilities are null");

		this->mapPayloadTypes.clear();

		for (auto& codec : this->rtpParameters->codecs)
		{
			auto it = this->peerCapabilities->codecs.begin();

			for (; it != this->peerCapabilities->codecs.end(); ++it)
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
			if (it == this->peerCapabilities->codecs.end())
			{
				// TODO: Let's see. Theoretically unsupported codecs has been
				// already removed in Send(), but it's not done yet.
				// Once done, this should be a MS_THROW.
				MS_ERROR("no matching room codec found [payloadType:%" PRIu8 "]",
					codec.payloadType);
			}
		}
	}
}
