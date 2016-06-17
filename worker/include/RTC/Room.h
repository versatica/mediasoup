#ifndef MS_RTC_ROOM_H
#define MS_RTC_ROOM_H

#include "common.h"
#include "RTC/RtpDictionaries.h"
#include "RTC/Peer.h"
#include "RTC/RtpReceiver.h"
#include "RTC/RtpSender.h"
#include "RTC/RtpPacket.h"
#include "RTC/RtcpPacket.h"
#include "Channel/Request.h"
#include "Channel/Notifier.h"
#include <unordered_map>
#include <unordered_set>
#include <json/json.h>

namespace RTC
{
	class Room :
		public RTC::Peer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onRoomClosed(RTC::Room* room) = 0;
		};

	public:
		static void ClassInit();

	private:
		static RTC::RtpCapabilities supportedRtpCapabilities;
		static std::vector<uint8_t> availablePayloadTypes;

	public:
		Room(Listener* listener, Channel::Notifier* notifier, uint32_t roomId, Json::Value& data);
		virtual ~Room();

		void Close();
		Json::Value toJson();
		void HandleRequest(Channel::Request* request);
		RTC::RtpCapabilities& GetRtpCapabilities();

	private:
		RTC::Peer* GetPeerFromRequest(Channel::Request* request, uint32_t* peerId = nullptr);
		void SetRtpCapabilities(std::vector<RTC::RtpCodecParameters>& mediaCodecs);

	/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		virtual void onPeerClosed(RTC::Peer* peer) override;
		virtual void onPeerRtpReceiverParameters(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) override;
		virtual void onPeerRtpReceiverParametersDone(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) override;
		virtual void onPeerRtpReceiverClosed(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) override;
		virtual void onPeerRtpSenderClosed(RTC::Peer* peer, RTC::RtpSender* rtpSender) override;
		virtual void onPeerRtpPacket(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) override;

	public:
		// Passed by argument.
		uint32_t roomId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		// Others.
		RTC::RtpCapabilities rtpCapabilities;
		std::unordered_map<uint32_t, RTC::Peer*> peers;
		std::unordered_map<RTC::RtpReceiver*, std::unordered_set<RTC::RtpSender*>> mapRtpReceiverRtpSenders;
	};

	/* Inline static methods. */

	inline
	RTC::RtpCapabilities& Room::GetRtpCapabilities()
	{
		return this->rtpCapabilities;
	}
}

#endif
