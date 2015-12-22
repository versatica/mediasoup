#define MS_CLASS "RTC::Room"

#include "RTC/Room.h"
#include "MediaSoupError.h"
#include "Logger.h"

// TMP
#define NUM_PEERS  4
#define PAYLOAD_TYPE_AUDIO  111  // OPUS
#define PAYLOAD_TYPE_VIDEO  100  // VP8
#define SSRC_AUDIO_BASE  100000
#define SSRC_VIDEO_BASE  200000

namespace RTC
{
	/* Class methods. */

	RTC::Room* Room::Factory(Json::Value& data)
	{
		MS_TRACE();

		return new Room();
	}

	/* Instance methods. */

	Room::Room()
	{
		MS_TRACE();

		// TMP
		// for (int i = 1; i <= NUM_PEERS; i++)
		// {
		// 	RTC::Peer* peer = new RTC::Peer(this);

		// 	// TODO: this is not being freed
		// 	int* data = new int(i);
		// 	peer->SetUserData((void*)data);

		// 	this->peers.push_back(peer);
		// }
	}

	Room::~Room()
	{
		MS_TRACE();
	}

	void Room::Close()
	{
		MS_TRACE();

		// Close all the Peers.
		for (auto& kv : this->peers)
		{
			RTC::Peer* peer = kv.second;

			peer->Close();
		}

		delete this;
	}

	void Room::onRTPPacket(RTC::Peer* peer, RTC::RTPPacket* packet)
	{
		MS_TRACE();

		// int peer_id = *(int*)peer->GetUserData();
		// MS_BYTE payload_type = packet->GetPayloadType();
		// MS_4BYTES original_ssrc = packet->GetSSRC();

		// MS_DEBUG("valid RTP packet received from Peer %d [ssrc: %llu | payload: %hu]",
		// 	peer_id, (unsigned long long)packet->GetSSRC(), (unsigned short)packet->GetPayloadType());

		// switch (payload_type)
		// {
		// 	case PAYLOAD_TYPE_AUDIO:
		// 	case PAYLOAD_TYPE_VIDEO:
		// 		break;
		// 	default:
		// 		MS_ERROR("payload is not OPUS %d nor VP8 %d, packet ignored", PAYLOAD_TYPE_AUDIO, PAYLOAD_TYPE_VIDEO);
		// 		return;
		// }

		// // Deliver to all the peers.
		// // TODO: but this one? yes (TODO)
		// for (auto dst_peer : this->peers)
		// {
		// 	// if (dst_peer == peer)
		// 	// 	continue;

		// 	int dst_peer_id = *(int*)dst_peer->GetUserData();

		// 	switch (payload_type)
		// 	{
		// 		case PAYLOAD_TYPE_AUDIO:
		// 			packet->SetSSRC((MS_4BYTES)(SSRC_AUDIO_BASE + (peer_id * 10) + dst_peer_id));
		// 			break;
		// 		case PAYLOAD_TYPE_VIDEO:
		// 			packet->SetSSRC((MS_4BYTES)(SSRC_VIDEO_BASE + (peer_id * 10) + dst_peer_id));
		// 			break;
		// 		default:
		// 			MS_ABORT("no puede ser!!!");
		// 			return;
		// 	}

		// 	MS_DEBUG("sending RTP packet to Peer %d [ssrc: %llu | payload: %hu | size: %zu]",
		// 		dst_peer_id, (unsigned long long)packet->GetSSRC(), (unsigned short)packet->GetPayloadType(), packet->GetLength());

		// 	dst_peer->SendRTPPacket(packet);

		// 	// NOTE: recover the previous SSRC since other peers are going to
		// 	// send this same RTPPacket!
		// 	packet->SetSSRC(original_ssrc);
		// }
	}

	void Room::onRTCPPacket(RTC::Peer* peer, RTC::RTCPPacket* packet)
	{
		MS_TRACE();

		// TMP

		// Ignore RTCP packets.

		// for (auto dst_peer : this->peers) {
			// dst_peer->SendRTCPPacket(packet);
		// }
	}
}
