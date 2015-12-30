#define MS_CLASS "RTC::Peer"

#include "RTC/Peer.h"
#include "RTC/DTLSRole.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	Peer::Peer(Listener* listener, std::string& peerId, Json::Value& data) :
		peerId(peerId),
		listener(listener)
	{
		MS_TRACE();

		// TODO: do something wit data and throw if incorrect.
	}

	Peer::~Peer()
	{
		MS_TRACE();
	}

	void Peer::Close()
	{
		MS_TRACE();

		// Close all the Transports.
		for (auto& kv : this->transports)
		{
			RTC::Transport* transport = kv.second;

			transport->Close();
		}

		// Notify the listener.
		this->listener->onPeerClosed(this);

		delete this;
	}

	Json::Value Peer::Dump()
	{
		MS_TRACE();

		Json::Value json(Json::objectValue);

		// TODO

		return json;
	}

	void Peer::HandleRequest(Channel::Request* request)
	{
		MS_TRACE();

		switch (request->methodId)
		{
			case Channel::Request::MethodId::createTransport:
			{
				RTC::Transport* transport;
				unsigned int transportId;

				try
				{
					transport = GetTransportFromRequest(request, &transportId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				if (transport)
				{
					MS_ERROR("Transport already exists");

					request->Reject(500, "Transport already exists");
					return;
				}

				try
				{
					transport = new RTC::Transport(this, transportId, request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				this->transports[transportId] = transport;

				MS_DEBUG("Transport created [transportId:%u]", transportId);

				auto data = transport->toJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::createAssociatedTransport:
			{
				RTC::Transport* transport;
				unsigned int transportId;

				try
				{
					transport = GetTransportFromRequest(request, &transportId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				if (transport)
				{
					MS_ERROR("Transport already exists");

					request->Reject(500, "Transport already exists");
					return;
				}

				auto jsonRtpTransportId = request->data["rtpTransportId"];

				if (!jsonRtpTransportId.isUInt())
				{
					MS_ERROR("Request has not numeric .rtpTransportId field");

					request->Reject(500, "Request has not numeric .rtpTransportId field");
					return;
				}

				auto it = this->transports.find(jsonRtpTransportId.asUInt());

				if (it == this->transports.end())
				{
					MS_ERROR("RTP Transport does not exist");

					request->Reject(500, "RTP Transport does not exist");
					return;
				}

				RTC::Transport* rtpTransport = it->second;

				try
				{
					transport = rtpTransport->CreateAssociatedTransport(transportId);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				this->transports[transportId] = transport;

				MS_DEBUG("Associated Transport created [transportId:%u]", transportId);

				auto data = transport->toJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::closeTransport:
			{
				// TODO

				request->Accept();

				break;
			}

			default:
			{
				MS_ABORT("unknown method");
			}
		}
	}

	RTC::Transport* Peer::GetTransportFromRequest(Channel::Request* request, unsigned int* transportId)
	{
		MS_TRACE();

		auto jsonTransportId = request->data["transportId"];

		if (!jsonTransportId.isUInt())
			MS_THROW_ERROR("Request has not numeric .transportId field");

		// If given, fill roomId.
		if (transportId)
			*transportId = jsonTransportId.asUInt();

		auto it = this->transports.find(jsonTransportId.asUInt());

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
}
