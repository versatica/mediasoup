#define MS_CLASS "RTC::DirectTransport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/DirectTransport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "PayloadChannel/PayloadChannelNotifier.hpp"

namespace RTC
{
	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	DirectTransport::DirectTransport(const std::string& id, RTC::Transport::Listener* listener, json& data)
	  : RTC::Transport::Transport(id, listener, data)
	{
		MS_TRACE();
	}

	DirectTransport::~DirectTransport()
	{
		MS_TRACE();
	}

	void DirectTransport::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJson(jsonObject);
	}

	void DirectTransport::FillJsonStats(json& jsonArray)
	{
		MS_TRACE();

		// Call the parent method.
		RTC::Transport::FillJsonStats(jsonArray);

		auto& jsonObject = jsonArray[0];

		// Add type.
		jsonObject["type"] = "direct-transport";
	}

	void DirectTransport::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		// Pass it to the parent class.
		RTC::Transport::HandleRequest(request);
	}

	void DirectTransport::HandleNotification(PayloadChannel::Notification* notification)
	{
		MS_TRACE();

		switch (notification->eventId)
		{
			case PayloadChannel::Notification::EventId::TRANSPORT_SEND_RTCP:
			{
				const auto* data = notification->payload;
				auto len         = notification->payloadLen;

				// Increase receive transmission.
				RTC::Transport::DataReceived(len);

				if (len > RTC::MtuSize + 100)
				{
					MS_WARN_TAG(rtp, "given RTCP packet exceeds maximum size [len:%zu]", len);

					return;
				}

				RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(data, len);

				if (!packet)
				{
					MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

					return;
				}

				// Pass the packet to the parent transport.
				RTC::Transport::ReceiveRtcpPacket(packet);

				break;
			}

			case PayloadChannel::Notification::EventId::PRODUCER_SEND:
			{
				const auto* data = notification->payload;
				auto len         = notification->payloadLen;

				// Increase receive transmission.
				RTC::Transport::DataReceived(len);

				if (len > RTC::MtuSize + 100)
				{
					MS_WARN_TAG(rtp, "given RTP packet exceeds maximum size [len:%zu]", len);

					return;
				}

				RTC::RtpPacket* packet = RTC::RtpPacket::Parse(data, len);

				if (!packet)
				{
					MS_WARN_TAG(rtp, "received data is not a valid RTP packet");

					return;
				}

				// Pass the packet to the parent transport.
				RTC::Transport::ReceiveRtpPacket(packet);

				break;
			}

			case PayloadChannel::Notification::EventId::DATA_PRODUCER_SEND:
			{
				// This may throw.
				RTC::DataProducer* dataProducer = GetDataProducerFromInternal(notification->internal);

				auto jsonPpidIt = notification->data.find("ppid");

				if (jsonPpidIt == notification->data.end() || !Utils::Json::IsPositiveInteger(*jsonPpidIt))
				{
					MS_THROW_TYPE_ERROR("invalid ppid");
				}

				auto ppid       = jsonPpidIt->get<uint32_t>();
				const auto* msg = notification->payload;
				auto len        = notification->payloadLen;

				if (len > this->maxMessageSize)
				{
					MS_WARN_TAG(
					  message,
					  "given message exceeds maxMessageSize value [maxMessageSize:%zu, len:%zu]",
					  len,
					  this->maxMessageSize);

					return;
				}

				dataProducer->ReceiveMessage(ppid, msg, len);

				// Increase receive transmission.
				RTC::Transport::DataReceived(len);

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Transport::HandleNotification(notification);
			}
		}
	}

	inline bool DirectTransport::IsConnected() const
	{
		return true;
	}

	void DirectTransport::SendRtpPacket(
	  RTC::Consumer* consumer, RTC::RtpPacket* packet, RTC::Transport::onSendCallback* cb)
	{
		MS_TRACE();

		if (!consumer)
		{
			MS_WARN_TAG(rtp, "cannot send RTP packet not associated to a Consumer");

			return;
		}

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// Notify the Node DirectTransport.
		PayloadChannel::PayloadChannelNotifier::Emit(consumer->id, "rtp", data, len);

		if (cb)
		{
			(*cb)(true);
			delete cb;
		}

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void DirectTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// Notify the Node DirectTransport.
		PayloadChannel::PayloadChannelNotifier::Emit(this->id, "rtcp", data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void DirectTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		const uint8_t* data = packet->GetData();
		size_t len          = packet->GetSize();

		// Notify the Node DirectTransport.
		PayloadChannel::PayloadChannelNotifier::Emit(this->id, "rtcp", data, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void DirectTransport::SendMessage(
	  RTC::DataConsumer* dataConsumer, uint32_t ppid, const uint8_t* msg, size_t len, onQueuedCallback* cb)
	{
		MS_TRACE();

		// Notify the Node DirectTransport.
		json data = json::object();

		data["ppid"] = ppid;

		PayloadChannel::PayloadChannelNotifier::Emit(dataConsumer->id, "message", data, msg, len);

		// Increase send transmission.
		RTC::Transport::DataSent(len);
	}

	void DirectTransport::SendSctpData(const uint8_t* /*data*/, size_t /*len*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void DirectTransport::RecvStreamClosed(uint32_t /*ssrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}

	void DirectTransport::SendStreamClosed(uint32_t /*ssrc*/)
	{
		MS_TRACE();

		// Do nothing.
	}
} // namespace RTC
