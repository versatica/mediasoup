#define MS_CLASS "RTC::Peer"

#include "RTC/Peer.h"
#include "RTC/RtpKind.h"
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

		static const Json::StaticString k_class("class");

		Json::Value event_data(Json::objectValue);

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

		// Notify.
		event_data[k_class] = "Peer";
		this->notifier->Emit(this->peerId, "close", event_data);

		// Notify the listener.
		this->listener->onPeerClosed(this);

		delete this;
	}

	Json::Value Peer::toJson()
	{
		MS_TRACE();

		static const Json::StaticString k_peerId("peerId");
		static const Json::StaticString k_peerName("peerName");
		static const Json::StaticString k_transports("transports");
		static const Json::StaticString k_rtpReceivers("rtpReceivers");
		static const Json::StaticString k_rtpSenders("rtpSenders");

		Json::Value json(Json::objectValue);
		Json::Value json_transports(Json::objectValue);
		Json::Value json_rtpReceivers(Json::objectValue);
		Json::Value json_rtpSenders(Json::objectValue);

		json[k_peerId] = (Json::UInt)this->peerId;

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

		for (auto& kv : this->rtpSenders)
		{
			RTC::RtpSender* rtpSender = kv.second;

			json_rtpSenders[std::to_string(rtpSender->rtpSenderId)] = rtpSender->toJson();
		}
		json[k_rtpSenders] = json_rtpSenders;

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
				static const Json::StaticString k_kind("kind");

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

				// `kind` is mandatory.

				if (!request->data[k_kind].isString())
					MS_THROW_ERROR("missing `kind`");

				std::string kind = request->data[k_kind].asString();

				// Create a RtpReceiver instance.
				try
				{
					rtpReceiver = new RTC::RtpReceiver(this, this->notifier, rtpReceiverId, kind);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(error.what());
					return;
				}

				this->rtpReceivers[rtpReceiverId] = rtpReceiver;

				MS_DEBUG("RtpReceiver created [rtpReceiverId:%" PRIu32 "]", rtpReceiverId);

				// Set the Transport.
				rtpReceiver->SetTransport(transport);

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
			case Channel::Request::MethodId::rtpReceiver_listenForRtp:
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

			case Channel::Request::MethodId::rtpSender_dump:
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

			case Channel::Request::MethodId::rtpSender_setTransport:
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

				rtpSender->SetTransport(transport);

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

	void Peer::AddRtpSender(RTC::RtpSender* rtpSender, std::string& peerName)
	{
		MS_TRACE();

		static const Json::StaticString k_rtpSenderId("rtpSenderId");
		static const Json::StaticString k_kind("kind");
		static const Json::StaticString v_audio("audio");
		static const Json::StaticString v_video("video");
		static const Json::StaticString k_rtpParameters("rtpParameters");
		static const Json::StaticString k_peerName("peerName");

		MS_ASSERT(this->rtpSenders.find(rtpSender->rtpSenderId) == this->rtpSenders.end(),
			"given RtpSender already exists in this Peer");

		MS_ASSERT(rtpSender->GetRtpParameters(), "given RtpSender does not have RtpParameters");

		// Store it.
		this->rtpSenders[rtpSender->rtpSenderId] = rtpSender;

		// Notify.

		Json::Value event_data(Json::objectValue);

		event_data[k_rtpSenderId] = (Json::UInt)rtpSender->rtpSenderId;
		switch (rtpSender->kind)
		{
			case RTC::RtpKind::AUDIO:
				event_data[k_kind] = v_audio;
				break;
			case RTC::RtpKind::VIDEO:
				event_data[k_kind] = v_video;
				break;
		}
		event_data[k_rtpParameters] = rtpSender->GetRtpParameters()->toJson();
		event_data[k_peerName] = peerName;

		this->notifier->Emit(this->peerId, "newrtpsender", event_data);
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

		// Must remove the closed Transport from all the RtpReceivers holding it.
		for (auto& kv : this->rtpReceivers)
		{
			RTC::RtpReceiver* rtpReceiver = kv.second;

			rtpReceiver->RemoveTransport(transport);
		}

		// Must also unset this Transport from all the RtpSenders using it.
		for (auto& kv : this->rtpSenders)
		{
			RTC::RtpSender* rtpSender = kv.second;

			rtpSender->RemoveTransport(transport);
		}

		this->transports.erase(transport->transportId);
	}

	void Peer::onRtpReceiverParameters(RTC::RtpReceiver* rtpReceiver, RTC::RtpParameters* rtpParameters)
	{
		MS_TRACE();

		// Notify the listener (Room) so it can check provided codecs and,
		// optionally, normalize them.
		// NOTE: This may throw.
		this->listener->onPeerRtpReceiverParameters(this, rtpParameters);

		auto transport = rtpReceiver->GetTransport();

		// NOTE: This may throw.
		if (transport)
			transport->AddRtpReceiver(rtpReceiver, rtpParameters);

		// Notify the listener (Room).
		this->listener->onPeerRtpReceiverParametersDone(this, rtpReceiver);
	}

	void Peer::onRtpPacket(RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->onPeerRtpPacket(this, rtpReceiver, packet);
	}

	void Peer::onRtpReceiverClosed(RTC::RtpReceiver* rtpReceiver)
	{
		MS_TRACE();

		// We must remove the closed RtpReceiver from the Transports holding it.
		for (auto& kv : this->transports)
		{
			auto transport = kv.second;

			transport->RemoveRtpReceiver(rtpReceiver);
		}

		// Remove from the map.
		this->rtpReceivers.erase(rtpReceiver->rtpReceiverId);

		// Notify the listener (Room) so it can remove this RtpReceiver from its map.
		this->listener->onPeerRtpReceiverClosed(this, rtpReceiver);
	}

	void Peer::onRtpSenderClosed(RTC::RtpSender* rtpSender)
	{
		MS_TRACE();

		// Remove from the map.
		this->rtpSenders.erase(rtpSender->rtpSenderId);

		// Notify the listener (Room) so it can remove this RtpSender from its map.
		this->listener->onPeerRtpSenderClosed(this, rtpSender);
	}
}
