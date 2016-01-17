#define MS_CLASS "RTC::Peer"

#include "RTC/Peer.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	Peer::Peer(Listener* listener, Channel::Notifier* notifier, uint32_t peerId, std::string& peerName) :
		peerId(peerId),
		peerName(peerName),
		listener(listener),
		notifier(notifier)
	{
		MS_TRACE();
	}

	Peer::~Peer()
	{
		MS_TRACE();
	}

	void Peer::Close()
	{
		MS_TRACE();

		// Close all the RtpReceivers.
		for (auto it = this->rtpReceivers.begin(); it != this->rtpReceivers.end();)
		{
			RTC::RtpReceiver* rtpReceiver = it->second;

			it = this->rtpReceivers.erase(it);
			rtpReceiver->Close();
		}

		// Close all the Transports.
		// NOTE: It is critical to close Transports after RtpReceivers because
		// RtcReceiver.Close() fires an event in the Transport.
		for (auto it = this->transports.begin(); it != this->transports.end();)
		{
			RTC::Transport* transport = it->second;

			it = this->transports.erase(it);
			transport->Close();
		}

		// Notify the listener.
		this->listener->onPeerClosed(this);

		delete this;
	}

	Json::Value Peer::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_peerName("peerName");
		static const Json::StaticString k_transports("transports");
		static const Json::StaticString k_rtpReceivers("rtpReceivers");

		Json::Value json(Json::objectValue);
		Json::Value json_transports(Json::objectValue);
		Json::Value json_rtpReceivers(Json::objectValue);

		json[k_peerName] = this->peerName;

		for (auto& kv : this->transports)
		{
			RTC::Transport* transport = kv.second;

			json_transports[std::to_string(transport->transportId)] = transport->toJson();
		}
		json[k_transports] = json_transports;

		for (auto& kv : this->rtpReceivers)
		{
			RTC::RtpReceiver* rtpReceiver = kv.second;

			json_rtpReceivers[std::to_string(rtpReceiver->rtpReceiverId)] = rtpReceiver->toJson();
		}
		json[k_rtpReceivers] = json_rtpReceivers;

		return json;
	}

	void Peer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::peer_close:
			{
				uint32_t peerId = this->peerId;

				Close();

				MS_DEBUG("Peer closed [peerId:%" PRIu32 "]", peerId);
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::peer_dump:
			{
				Json::Value json = toJson();

				request->Accept(json);

				break;
			}

			case Channel::Request::MethodId::peer_createTransport:
			{
				RTC::Transport* transport;
				uint32_t transportId;

				try
				{
					transport = GetTransportFromRequest(request, &transportId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (transport)
				{
					MS_ERROR("Transport already exists");

					request->Reject("Transport already exists");
					return;
				}

				try
				{
					transport = new RTC::Transport(this, this->notifier, transportId, request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				this->transports[transportId] = transport;

				MS_DEBUG("Transport created [transportId:%" PRIu32 "]", transportId);

				auto data = transport->toJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::peer_createAssociatedTransport:
			{
				RTC::Transport* transport;
				RTC::Transport* rtpTransport;
				uint32_t transportId;

				try
				{
					transport = GetTransportFromRequest(request, &transportId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (transport)
				{
					MS_ERROR("Transport already exists");

					request->Reject("Transport already exists");
					return;
				}

				static const Json::StaticString k_rtpTransportId("rtpTransportId");

				auto json_rtpTransportId = request->internal[k_rtpTransportId];

				if (!json_rtpTransportId.isUInt())
				{
					MS_ERROR("Request has not numeric `internal.rtpTransportId`");

					request->Reject("Request has not numeric `internal.rtpTransportId`");
					return;
				}

				auto it = this->transports.find(json_rtpTransportId.asUInt());
				if (it == this->transports.end())
				{
					MS_ERROR("RTP Transport does not exist");

					request->Reject("RTP Transport does not exist");
					return;
				}

				rtpTransport = it->second;

				try
				{
					transport = rtpTransport->CreateAssociatedTransport(transportId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				this->transports[transportId] = transport;

				MS_DEBUG("Associated Transport created [transportId:%" PRIu32 "]", transportId);

				auto data = transport->toJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::peer_createRtpReceiver:
			{
				RTC::RtpReceiver* rtpReceiver;
				RTC::Transport* transport = nullptr;
				RTC::Transport* rtcpTransport = nullptr;
				uint32_t rtpReceiverId;

				try
				{
					rtpReceiver = GetRtpReceiverFromRequest(request, &rtpReceiverId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (rtpReceiver)
				{
					MS_ERROR("RtpReceiver already exists");

					request->Reject("RtpReceiver already exists");
					return;
				}

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!transport)
				{
					MS_ERROR("Transport does not exist");

					request->Reject("Transport does not exist");
					return;
				}

				try
				{
					rtcpTransport = GetRtcpTransportFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!rtcpTransport)
				{
					MS_ERROR("RTCP Transport does not exist");

					request->Reject("RTCP Transport does not exist");
					return;
				}

				// Create a RtpReceiver instance.
				rtpReceiver = new RTC::RtpReceiver(this, this->notifier, rtpReceiverId);

				this->rtpReceivers[rtpReceiverId] = rtpReceiver;

				MS_DEBUG("RtpReceiver created [rtpReceiverId:%" PRIu32 "]", rtpReceiverId);

				// Set the transports as RTP and RTCP listeners.
				rtpReceiver->SetRtpListener(transport);
				if (rtcpTransport != transport)
					rtpReceiver->SetRtpListenerForRtcp(rtcpTransport);

				request->Accept();

				break;
			}

			case Channel::Request::MethodId::transport_close:
			case Channel::Request::MethodId::transport_dump:
			case Channel::Request::MethodId::transport_setRemoteDtlsParameters:
			{
				RTC::Transport* transport;

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!transport)
				{
					MS_ERROR("Transport does not exist");

					request->Reject("Transport does not exist");
					return;
				}

				transport->HandleRequest(request);

				break;
			}

			case Channel::Request::MethodId::rtpReceiver_close:
			case Channel::Request::MethodId::rtpReceiver_dump:
			case Channel::Request::MethodId::rtpReceiver_receive:
			{
				RTC::RtpReceiver* rtpReceiver;

				try
				{
					rtpReceiver = GetRtpReceiverFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!rtpReceiver)
				{
					MS_ERROR("RtpReceiver does not exist");

					request->Reject("RtpReceiver does not exist");
					return;
				}

				rtpReceiver->HandleRequest(request);

				break;
			}

			default:
			{
				MS_ERROR("unknown method");

				request->Reject("unknown method");
			}
		}
	}

	RTC::Transport* Peer::GetTransportFromRequest(Channel::Request* request, uint32_t* transportId)
	{
		MS_TRACE();

		static const Json::StaticString k_transportId("transportId");

		auto json_transportId = request->internal[k_transportId];

		if (!json_transportId.isUInt())
			MS_THROW_ERROR("Request has not numeric `internal.transportId`");

		if (transportId)
			*transportId = json_transportId.asUInt();

		auto it = this->transports.find(json_transportId.asUInt());
		if (it != this->transports.end())
		{
			RTC::Transport* transport = it->second;

			return transport;
		}
		else
		{
			return nullptr;
		}
	}

	RTC::Transport* Peer::GetRtcpTransportFromRequest(Channel::Request* request, uint32_t* rtcpTransportId)
	{
		MS_TRACE();

		static const Json::StaticString k_rtcpTransportId("rtcpTransportId");

		auto json_rtcpTransportId = request->internal[k_rtcpTransportId];

		if (!json_rtcpTransportId.isUInt())
			MS_THROW_ERROR("Request has not numeric `internal.rtcpTransportId`");

		if (rtcpTransportId)
			*rtcpTransportId = json_rtcpTransportId.asUInt();

		auto it = this->transports.find(json_rtcpTransportId.asUInt());
		if (it != this->transports.end())
		{
			RTC::Transport* rtcpTransport = it->second;

			return rtcpTransport;
		}
		else
		{
			return nullptr;
		}
	}

	RTC::RtpReceiver* Peer::GetRtpReceiverFromRequest(Channel::Request* request, uint32_t* rtpReceiverId)
	{
		MS_TRACE();

		static const Json::StaticString k_rtpReceiverId("rtpReceiverId");

		auto json_rtpReceiverId = request->internal[k_rtpReceiverId];

		if (!json_rtpReceiverId.isUInt())
			MS_THROW_ERROR("Request has not numeric `internal.rtpReceiverId`");

		if (rtpReceiverId)
			*rtpReceiverId = json_rtpReceiverId.asUInt();

		auto it = this->rtpReceivers.find(json_rtpReceiverId.asUInt());
		if (it != this->rtpReceivers.end())
		{
			RTC::RtpReceiver* rtpReceiver = it->second;

			return rtpReceiver;
		}
		else
		{
			return nullptr;
		}
	}

	void Peer::onTransportClosed(RTC::Transport* transport)
	{
		MS_TRACE();

		// If a Transport is closed we need to inform all the RtpReceivers of the
		// Peer since a RtpReceiver has one or two listeners which are Transports.
		for (auto& kv : this->rtpReceivers)
		{
			RTC::RtpReceiver* rtpReceiver = kv.second;

			rtpReceiver->RemoveRtpListener(transport);
		}

		this->transports.erase(transport->transportId);
	}

	void Peer::onRtpReceiverClosed(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		this->rtpReceivers.erase(rtpReceiver->rtpReceiverId);
	}
}
