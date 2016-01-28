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

		// Close all the RtpSenders.
		for (auto it = this->rtpSenders.begin(); it != this->rtpSenders.end();)
		{
			RTC::RtpSender* rtpSender = it->second;

			it = this->rtpSenders.erase(it);
			rtpSender->Close();
		}

		// Close all the Transports.
		// NOTE: It is critical to close Transports after RtpReceivers/RtpSenders
		// because RtcReceiver.Close() fires an event in the Transport.
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

			case Channel::Request::MethodId::peer_createRtpReceiver:
			{
				RTC::RtpReceiver* rtpReceiver;
				RTC::Transport* transport = nullptr;
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

				// Create a RtpReceiver instance.
				rtpReceiver = new RTC::RtpReceiver(this, this->notifier, rtpReceiverId);

				this->rtpReceivers[rtpReceiverId] = rtpReceiver;

				MS_DEBUG("RtpReceiver created [rtpReceiverId:%" PRIu32 "]", rtpReceiverId);

				// RTC::Transport inherits from RTC::RtpListener, but the API of
				// RTC:RtpReceiver requires RTP::RtpListener.
				RTC::RtpListener* rtpListener = transport;

				// Set the RtpListener.
				rtpReceiver->SetRtpListener(rtpListener);

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

			case Channel::Request::MethodId::rtpSender_close:
			case Channel::Request::MethodId::rtpSender_dump:
			case Channel::Request::MethodId::rtpSender_send:
			{
				RTC::RtpSender* rtpSender;

				try
				{
					rtpSender = GetRtpSenderFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				if (!rtpSender)
				{
					MS_ERROR("RtpSender does not exist");

					request->Reject("RtpSender does not exist");
					return;
				}

				rtpSender->HandleRequest(request);

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

	RTC::RtpSender* Peer::GetRtpSenderFromRequest(Channel::Request* request, uint32_t* rtpSenderId)
	{
		MS_TRACE();

		static const Json::StaticString k_rtpSenderId("rtpSenderId");

		auto json_rtpSenderId = request->internal[k_rtpSenderId];

		if (!json_rtpSenderId.isUInt())
			MS_THROW_ERROR("Request has not numeric `internal.rtpSenderId`");

		if (rtpSenderId)
			*rtpSenderId = json_rtpSenderId.asUInt();

		auto it = this->rtpSenders.find(json_rtpSenderId.asUInt());
		if (it != this->rtpSenders.end())
		{
			RTC::RtpSender* rtpSender = it->second;

			return rtpSender;
		}
		else
		{
			return nullptr;
		}
	}

	void Peer::onTransportClosed(RTC::Transport* transport)
	{
		MS_TRACE();

		RTC::RtpListener* rtpListener = transport;

		// We must remove the closed Transport (so RtpListener) from the
		// RtpReceivers holding it.
		for (auto& kv : this->rtpReceivers)
		{
			RTC::RtpReceiver* rtpReceiver = kv.second;

			rtpReceiver->RemoveRtpListener(rtpListener);
		}

		this->transports.erase(transport->transportId);
	}

	void Peer::onRtpReceiverParameters(RTC::RtpReceiver* rtpReceiver, RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		auto rtpListener = rtpReceiver->GetRtpListener();

		// TODO: This may throw
		if (rtpListener)
			rtpListener->AddRtpReceiver(rtpReceiver, rtpParameters);

		// NOTE: If it does not throw do this.

		// Notify the listener (Room).
		this->listener->onPeerRtpReceiverReady(this, rtpReceiver);
	}

	void Peer::onRtpReceiverClosed(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// We must remove the closed RtpReceiver from the RtpListeners holding it.
		for (auto& kv : this->transports)
		{
			RTC::RtpListener* rtpListener = kv.second;

			rtpListener->RemoveRtpReceiver(rtpReceiver);
		}

		// TODO: Must notify the listener (Room) so it can remove this RtpReceiver.

		this->rtpReceivers.erase(rtpReceiver->rtpReceiverId);
	}

	void Peer::onRtpSenderParameters(RTC::RtpSender* rtpSender, RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		// auto rtpListener = rtpSender->GetRtpListener();

		// // TODO: This may throw
		// if (rtpListener)
		// 	rtpListener->AddRtpSender(rtpSender, rtpParameters);

		// // NOTE: If it does not throw do this.

		// // Notify the listener (Room).
		// this->listener->onPeerRtpSenderReady(this, rtpSender);
	}

	void Peer::onRtpSenderClosed(RTC::RtpSender* rtpSender)
	{
		MS_TRACE();

		// We must remove the closed RtpSender from the RtpListeners holding it.
		// for (auto& kv : this->transports)
		// {
		// 	RTC::RtpListener* rtpListener = kv.second;

		// 	rtpListener->RemoveRtpSender(rtpSender);
		// }

		// TODO: Must notify the listener (Room) so it can remove this RtpSender.

		this->rtpSenders.erase(rtpSender->rtpSenderId);
	}
}
