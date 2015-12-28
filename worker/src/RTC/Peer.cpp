#define MS_CLASS "RTC::Peer"

#include "RTC/Peer.h"
#include "RTC/DTLSRole.h"
#include "MediaSoupError.h"
#include "Logger.h"
// TMP
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"

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

		// TMP
		if (this->iceTransport)
			this->iceTransport->Close();

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
				try
				{
					this->iceTransport = new RTC::IceTransport(this, request->data);
				}
				catch (const MediaSoupError &error)
				{
					request->Reject(500, error.what());
					return;
				}

				auto data = this->iceTransport->toJson();

				request->Accept(data);

				break;
			}

			case Channel::Request::MethodId::closeTransport:
			case Channel::Request::MethodId::dumpTransport:
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
}
