#ifndef MS_RTC_ROOM_HPP
#define MS_RTC_ROOM_HPP

#include "common.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/Peer.hpp"
#include "RTC/RtpReceiver.hpp"
#include "RTC/RtpSender.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "Channel/Request.hpp"
#include "Channel/Notifier.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
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

	private:
		virtual ~Room();

	public:
		void Destroy();
		Json::Value toJson() const;
		void HandleRequest(Channel::Request* request);
		const RTC::RtpCapabilities& GetCapabilities() const;

	private:
		RTC::Peer* GetPeerFromRequest(Channel::Request* request, uint32_t* peerId = nullptr) const;
		void SetCapabilities(std::vector<RTC::RtpCodecParameters>& mediaCodecs);

	/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		virtual void onPeerClosed(const RTC::Peer* peer) override;
		virtual void onPeerCapabilities(RTC::Peer* peer, RTC::RtpCapabilities* capabilities) override;
		virtual void onPeerRtpReceiverParameters(const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) override;
		virtual void onPeerRtpReceiverClosed(const RTC::Peer* peer, const RTC::RtpReceiver* rtpReceiver) override;
		virtual void onPeerRtpSenderClosed(const RTC::Peer* peer, RTC::RtpSender* rtpSender) override;
		virtual void onPeerRtpPacket(const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) override;
		virtual void onPeerRtcpReceiverReport(const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::ReceiverReport* report) override;
		virtual void onPeerRtcpFeedback(const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackPsPacket* packet) override;
		virtual void onPeerRtcpFeedback(const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackRtpPacket* packet) override;
		virtual void onPeerRtcpSenderReport(const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RTCP::SenderReport* report) override;
		virtual void onFullFrameRequired(RTC::Peer* peer, RTC::RtpSender* rtpSender) override;

	public:
		// Passed by argument.
		uint32_t roomId;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		// Others.
		RTC::RtpCapabilities capabilities;
		std::unordered_map<uint32_t, RTC::Peer*> peers;
		std::unordered_map<const RTC::RtpReceiver*, std::unordered_set<RTC::RtpSender*>> mapRtpReceiverRtpSenders;
		std::unordered_map<const RTC::RtpSender*, const RTC::RtpReceiver*> mapRtpSenderRtpReceiver;
	};

	/* Inline static methods. */

	inline
	const RTC::RtpCapabilities& Room::GetCapabilities() const
	{
		return this->capabilities;
	}
}

#endif
