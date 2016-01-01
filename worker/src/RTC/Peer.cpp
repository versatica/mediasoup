#define MS_CLASS "RTC::Peer"

#include "RTC/Peer.h"
#include "RTC/DTLSRole.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	Peer::Peer(Listener* listener, std::string& peerId) :
		peerId(peerId),
		listener(listener)
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

		// Close all the Transports.
		// NOTE: Upon Transport closure the onTransportClosed() method is called which
		// removes it from the map, so this is the safe way to iterate the mao
		// and remove elements.
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

		static const Json::StaticString k_transports("transports");

		Json::Value json(Json::objectValue);
		Json::Value json_transports(Json::objectValue);

		for (auto& kv : this->transports)
		{
			RTC::Transport* transport = kv.second;

			json_transports[std::to_string(transport->transportId)] = transport->toJson();
		}
		json[k_transports] = json_transports;

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

				static const Json::StaticString k_rtpTransportId("rtpTransportId");

				auto jsonRtpTransportId = request->data[k_rtpTransportId];

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

				if (!transport)
				{
					MS_ERROR("Transport does not exist");

					request->Reject(500, "Transport does not exist");
					return;
				}

				transport->Close();

				MS_DEBUG("Transport closed [transportId:%u]", transportId);
				request->Accept();

				break;
			}

			case Channel::Request::MethodId::dumpTransport:
			{
				Json::Value json;
				RTC::Transport* transport;

				try
				{
					transport = GetTransportFromRequest(request);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				if (!transport)
				{
					MS_ERROR("Transport does not exist");

					request->Reject(500, "Transport does not exist");
					return;
				}

				json = transport->toJson();

				request->Accept(json);

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

		static const Json::StaticString k_transportId("transportId");

		auto jsonTransportId = request->data[k_transportId];

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

	void Peer::onTransportClosed(RTC::Transport* transport)
	{
		MS_TRACE();

		this->transports.erase(transport->transportId);
	}
}
