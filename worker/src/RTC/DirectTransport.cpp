#include "FBS/directTransport_generated.h"
#define MS_CLASS "RTC::DirectTransport"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/DirectTransport.hpp"

namespace RTC
{
	/* Instance methods. */

	// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
	DirectTransport::DirectTransport(
	  RTC::Shared* shared,
	  const std::string& id,
	  RTC::Transport::Listener* listener,
	  const FBS::DirectTransport::DirectTransportOptions* options)
	  : RTC::Transport::Transport(shared, id, listener, options->base())
	{
		MS_TRACE();

		// NOTE: This may throw.
		this->shared->channelMessageRegistrator->RegisterHandler(
		  this->id,
		  /*channelRequestHandler*/ this,
		  /*channelNotificationHandler*/ this);
	}

	DirectTransport::~DirectTransport()
	{
		MS_TRACE();

		this->shared->channelMessageRegistrator->UnregisterHandler(this->id);
	}

	flatbuffers::Offset<FBS::DirectTransport::DirectTransportDumpResponse> DirectTransport::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		// Add base transport dump.
		auto base = Transport::FillBuffer(builder);

		return FBS::DirectTransport::CreateDirectTransportDumpResponse(builder, base);
	}

	flatbuffers::Offset<FBS::Transport::GetStatsResponse> DirectTransport::FillBufferStats(
	  flatbuffers::FlatBufferBuilder& builder)
	{
		MS_TRACE();

		// Base Transport stats.
		auto base = Transport::FillBufferStats(builder);
		// DirectTransport stats.
		auto directTransportStats = FBS::Transport::CreateDirectTransportStats(builder, base);

		return FBS::Transport::CreateGetStatsResponse(
		  builder, FBS::Transport::StatsData::DirectTransportStats, directTransportStats.Union());
	}

	void DirectTransport::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->method)
		{
			case Channel::ChannelRequest::Method::TRANSPORT_DUMP:
			{
				auto dumpOffset = FillBuffer(request->GetBufferBuilder());

				request->Accept(FBS::Response::Body::FBS_PipeTransport_PipeTransportDumpResponse, dumpOffset);

				break;
			}

			default:
			{
				// Pass it to the parent class.
				RTC::Transport::HandleRequest(request);
			}
		}
	}

	void DirectTransport::HandleNotification(Channel::ChannelNotification* notification)
	{
		MS_TRACE();

		switch (notification->event)
		{
			case Channel::ChannelNotification::Event::TRANSPORT_SEND_RTCP:
			{
				auto body = notification->data->body_as<FBS::Transport::SendRtcpNotification>();
				auto len  = body->data()->size();

				// Increase receive transmission.
				RTC::Transport::DataReceived(len);

				if (len > RTC::MtuSize + 100)
				{
					MS_WARN_TAG(rtp, "given RTCP packet exceeds maximum size [len:%i]", len);

					return;
				}

				RTC::RTCP::Packet* packet = RTC::RTCP::Packet::Parse(body->data()->data(), len);

				if (!packet)
				{
					MS_WARN_TAG(rtcp, "received data is not a valid RTCP compound or single packet");

					return;
				}

				// Pass the packet to the parent transport.
				RTC::Transport::ReceiveRtcpPacket(packet);

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

		auto data = this->shared->channelNotifier->GetBufferBuilder().CreateVector(
		  packet->GetData(), packet->GetSize());

		auto notification =
		  FBS::Consumer::CreateRtpNotification(this->shared->channelNotifier->GetBufferBuilder(), data);

		this->shared->channelNotifier->Emit(
		  consumer->id,
		  FBS::Notification::Event::CONSUMER_RTP,
		  FBS::Notification::Body::FBS_Consumer_RtpNotification,
		  notification);

		if (cb)
		{
			(*cb)(true);
			delete cb;
		}

		// Increase send transmission.
		RTC::Transport::DataSent(packet->GetSize());
	}

	void DirectTransport::SendRtcpPacket(RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		// Notify the Node DirectTransport.
		auto data = this->shared->channelNotifier->GetBufferBuilder().CreateVector(
		  packet->GetData(), packet->GetSize());

		auto notification = FBS::DirectTransport::CreateRtcpNotification(
		  this->shared->channelNotifier->GetBufferBuilder(), data);

		this->shared->channelNotifier->Emit(
		  this->id,
		  FBS::Notification::Event::DIRECTTRANSPORT_RTCP,
		  FBS::Notification::Body::FBS_DirectTransport_RtcpNotification,
		  notification);

		// Increase send transmission.
		RTC::Transport::DataSent(packet->GetSize());
	}

	void DirectTransport::SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet)
	{
		MS_TRACE();

		packet->Serialize(RTC::RTCP::Buffer);

		auto data = this->shared->channelNotifier->GetBufferBuilder().CreateVector(
		  packet->GetData(), packet->GetSize());

		auto notification = FBS::DirectTransport::CreateRtcpNotification(
		  this->shared->channelNotifier->GetBufferBuilder(), data);

		this->shared->channelNotifier->Emit(
		  this->id,
		  FBS::Notification::Event::DIRECTTRANSPORT_RTCP,
		  FBS::Notification::Body::FBS_DirectTransport_RtcpNotification,
		  notification);
	}

	void DirectTransport::SendMessage(
	  RTC::DataConsumer* dataConsumer, uint32_t ppid, const uint8_t* msg, size_t len, onQueuedCallback* cb)
	{
		MS_TRACE();

		// Notify the Node DirectTransport.
		auto data = this->shared->channelNotifier->GetBufferBuilder().CreateVector(msg, len);

		auto notification = FBS::DataConsumer::CreateMessageNotification(
		  this->shared->channelNotifier->GetBufferBuilder(), ppid, data);

		this->shared->channelNotifier->Emit(
		  dataConsumer->id,
		  FBS::Notification::Event::DATACONSUMER_MESSAGE,
		  FBS::Notification::Body::FBS_DataConsumer_MessageNotification,
		  notification);

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
