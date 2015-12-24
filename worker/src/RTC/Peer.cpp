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
		listener(listener),
		peerId(peerId)
	{
		MS_TRACE();

		// TODO: do something wit data and throw if incorrect.
	}

	Peer::~Peer()
	{
		MS_TRACE();
	}

	void Peer::SendRTPPacket(RTC::RTPPacket* packet)
	{
		MS_TRACE();

		// TMP

		if (this->transport->IsReadyForMedia())
			this->transport->SendRTPPacket(packet);
	}

	void Peer::SendRTCPPacket(RTC::RTCPPacket* packet)
	{
		MS_TRACE();

		// TMP

		if (this->transport->IsReadyForMedia())
			this->transport->SendRTCPPacket(packet);
	}

	Json::Value Peer::Dump()
	{
		MS_TRACE();

		Json::Value json(Json::objectValue);

		// TODO: TMP
		json["foo"] = "bar";

		return json;
	}

	void Peer::Close()
	{
		MS_TRACE();

		// TMP
		if (this->transport)
			this->transport->Close();

		delete this;
	}

	void Peer::onRTPPacket(RTC::Transport* transport, RTC::RTPPacket* packet)
	{
		MS_TRACE();

		// MS_DEBUG("RTP packet received");  // TODO: TMP

		// TMP

		this->listener->onRTPPacket(this, packet);
	}

	void Peer::onRTCPPacket(RTC::Transport* transport, RTC::RTCPPacket* packet)
	{
		MS_TRACE();

		// MS_DEBUG("RTCP packet received");  // TODO: TMP

		// TMP

		this->listener->onRTCPPacket(this, packet);
	}
}
