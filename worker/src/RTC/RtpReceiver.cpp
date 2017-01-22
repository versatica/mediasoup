#define MS_CLASS "RTC::RtpReceiver"
// #define MS_LOG_DEV

#include "RTC/RtpReceiver.h"
#include "RTC/Transport.h"
#include "Utils.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Class variables. */

	uint8_t RtpReceiver::rtcpBuffer[MS_RTCP_BUFFER_SIZE];

	/* Instance methods. */

	RtpReceiver::RtpReceiver(Listener* listener, Channel::Notifier* notifier, uint32_t rtpReceiverId, RTC::Media::Kind kind) :
		rtpReceiverId(rtpReceiverId),
		kind(kind),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();
	}

	RtpReceiver::~RtpReceiver()
	{
		MS_TRACE();
	}

	void RtpReceiver::Close()
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");

		Json::Value event_data(Json::objectValue);

		if (this->rtpParameters)
			delete this->rtpParameters;

		if (this->rtpStream)
			delete this->rtpStream;

		// Notify.
		event_data[k_class] = "RtpReceiver";
		this->notifier->Emit(this->rtpReceiverId, "close", event_data);

		// Notify the listener.
		this->listener->onRtpReceiverClosed(this);

		delete this;
	}

	Json::Value RtpReceiver::toJson()
	{
		MS_TRACE();

		static Json::Value null_data(Json::nullValue);
		static const Json::StaticString k_rtpReceiverId("rtpReceiverId");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_hasTransport("hasTransport");
		static const Json::StaticString k_rtpRawEventEnabled("rtpRawEventEnabled");
		static const Json::StaticString k_rtpObjectEventEnabled("rtpObjectEventEnabled");

		Json::Value json(Json::objectValue);

		json[k_rtpReceiverId] = (Json::UInt)this->rtpReceiverId;

		json[k_kind] = RTC::Media::GetJsonString(this->kind);

		if (this->rtpParameters)
			json[k_rtpParameters] = this->rtpParameters->toJson();
		else
			json[k_rtpParameters] = null_data;

		json[k_hasTransport] = this->transport ? true : false;

		json[k_rtpRawEventEnabled] = this->rtpRawEventEnabled;

		json[k_rtpObjectEventEnabled] = this->rtpObjectEventEnabled;

		return json;
	}

	void RtpReceiver::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::rtpReceiver_close:
			{
				#ifdef MS_LOG_DEV
				uint32_t rtpReceiverId = this->rtpReceiverId;
				#endif

				Close();

				MS_DEBUG_DEV("RtpReceiver closed [rtpReceiverId:%" PRIu32 "]", rtpReceiverId);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_receive:
			{
				// Keep a reference to the previous rtpParameters.
				auto previousRtpParameters = this->rtpParameters;

				try
				{
					this->rtpParameters = new RTC::RtpParameters(request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());

					return;
				}

				// NOTE: this may throw. If so keep the current parameters.
				try
				{
					this->listener->onRtpReceiverParameters(this);
				}
				catch (const MediaSoupError &error)
				{
					// Rollback previous parameters.
					this->rtpParameters = previousRtpParameters;

					request->Reject(error.what());

					return;
				}

				// Free the previous rtpParameters.
				if (previousRtpParameters)
					delete previousRtpParameters;

				Json::Value data = this->rtpParameters->toJson();

				request->Accept(data);

				// Fill RTP parameters.
				FillRtpParameters();

				// And notify again.
				this->listener->onRtpReceiverParametersDone(this);

				// Set the RtpStream.
				// TODO: This assumes a single stream for now.
				// TODO: We need a much better way to get the clock rate.

				uint8_t streamPayloadType = this->rtpParameters->encodings[0].codecPayloadType;
				uint32_t streamClockRate;

				auto it = this->rtpParameters->codecs.begin();

				for (; it != this->rtpParameters->codecs.end(); ++it)
				{
					auto& codec = *it;

					if (codec.payloadType == streamPayloadType)
					{
						streamClockRate = codec.clockRate;

						break;
					}
				}
				// This should never happen.
				if (it == this->rtpParameters->codecs.end())
				{
					MS_ABORT("no valid codec payload type found for the first encoding");
				}

				switch (this->kind)
				{
					case RTC::Media::Kind::VIDEO:
					case RTC::Media::Kind::DEPTH:
						// Buffer up to 200 packets.
						this->rtpStream = new RTC::RtpStream(streamClockRate, 200);
						break;
					case RTC::Media::Kind::AUDIO:
						// No buffer for audio streams.
						this->rtpStream = new RTC::RtpStream(streamClockRate, 0);
						break;
					default:
						;
				}

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_setRtpRawEvent:
			{
				static const Json::StaticString k_enabled("enabled");

				if (!request->data[k_enabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				this->rtpRawEventEnabled = request->data[k_enabled].asBool();

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_setRtpObjectEvent:
			{
				static const Json::StaticString k_enabled("enabled");

				if (!request->data[k_enabled].isBool())
				{
					request->Reject("Request has invalid data.enabled");

					return;
				}

				this->rtpObjectEventEnabled = request->data[k_enabled].asBool();

				request->Accept();

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	void RtpReceiver::ReceiveRtpPacket(RTC::RtpPacket* packet)
	{
		MS_TRACE();

		static const Json::StaticString k_class("class");
		static const Json::StaticString k_object("object");
		static const Json::StaticString k_payloadType("payloadType");
		static const Json::StaticString k_marker("marker");
		static const Json::StaticString k_sequenceNumber("sequenceNumber");
		static const Json::StaticString k_timestamp("timestamp");
		static const Json::StaticString k_ssrc("ssrc");

		// TODO: Check if stopped, etc (not yet done).

		// TODO: Handle multiple streams.
		MS_ASSERT(this->rtpStream, "no RtpStream set");

		// Process the packet.
		// TODO: Must check what kind of packet we are checking. For example, RTX
		// packets (once implemented) should have a different handling.
		if (!this->rtpStream->ReceivePacket(packet))
			return;

		// Notify the listener.
		this->listener->onRtpPacket(this, packet);

		// Emit "rtpraw" if enabled.
		if (this->rtpRawEventEnabled)
		{
			Json::Value event_data(Json::objectValue);

			event_data[k_class] = "RtpReceiver";

			this->notifier->EmitWithBinary(this->rtpReceiverId, "rtpraw", event_data, packet->GetData(), packet->GetSize());
		}

		// Emit "rtpobject" is enabled.
		if (this->rtpObjectEventEnabled)
		{
			Json::Value event_data(Json::objectValue);
			Json::Value json_object(Json::objectValue);

			event_data[k_class] = "RtpReceiver";

			json_object[k_payloadType] = (Json::UInt)packet->GetPayloadType();
			json_object[k_marker] = packet->HasMarker();
			json_object[k_sequenceNumber] = (Json::UInt)packet->GetSequenceNumber();
			json_object[k_timestamp] = (Json::UInt)packet->GetTimestamp();
			json_object[k_ssrc] = (Json::UInt)packet->GetSsrc();

			event_data[k_object] = json_object;

			this->notifier->EmitWithBinary(this->rtpReceiverId, "rtpobject", event_data, packet->GetPayload(), packet->GetPayloadLength());
		}
	}

	void RtpReceiver::RequestRtpRetransmission(uint16_t seq, uint16_t bitmask, std::vector<RTC::RtpPacket*>& container)
	{
		MS_TRACE();

		// Proxy the request to the RtpStream.
		if (this->rtpStream)
			this->rtpStream->RequestRtpRetransmission(seq, bitmask, container);
	}

	void RtpReceiver::FillRtpParameters()
	{
		MS_TRACE();

		// Set a random muxId.
		// TODO: This is wrong, we are replacing the given muxId. Instead, this
		// method should set a random muxId and map the original value.
		this->rtpParameters->muxId = Utils::Crypto::GetRandomString(8);

		// TODO: Fill SSRCs with random values and set some mechanism to replace
		// SSRC values in received RTP packets to match the chosen random values.
	}

	RTC::RTCP::ReceiverReport* RtpReceiver::GetRtcpReceiverReport()
	{
		if (this->rtpStream)
		{
			RTC::RTCP::ReceiverReport* report = this->rtpStream->GetRtcpReceiverReport();

			// TODO: This assumes a single stream for now.
			report->SetSsrc(this->rtpParameters->encodings[0].ssrc);

			return report;
		}
		else
		{
			return nullptr;
		}
	}

	void RtpReceiver::ReceiveRtcpFeedback(RTC::RTCP::FeedbackPsPacket* packet)
	{
		MS_TRACE();

		if (this->transport)
		{
			// Ensure that the RTCP packet fits into the RTCP buffer.
			if (packet->GetSize() > MS_RTCP_BUFFER_SIZE)
			{
				MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)",
					packet->GetSize());

				return;
			}

			packet->Serialize(RtpReceiver::rtcpBuffer);
			this->transport->SendRtcpPacket(packet);
		}
	}

	void RtpReceiver::ReceiveRtcpFeedback(RTC::RTCP::FeedbackRtpPacket* packet)
	{
		MS_TRACE();

		if (this->transport)
		{
			// Ensure that the RTCP packet fits into the RTCP buffer.
			if (packet->GetSize() > MS_RTCP_BUFFER_SIZE)
			{
				MS_WARN_TAG(rtcp, "cannot send RTCP packet, size too big (%zu bytes)",
					packet->GetSize());

				return;
			}

			packet->Serialize(RtpReceiver::rtcpBuffer);
			this->transport->SendRtcpPacket(packet);
		}
	}
}
