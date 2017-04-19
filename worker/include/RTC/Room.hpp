#ifndef MS_RTC_ROOM_HPP
#define MS_RTC_ROOM_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/Peer.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpReceiver.hpp"
#include "RTC/RtpSender.hpp"
#include <json/json.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RTC
{
	class Room : public RTC::Peer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnRoomClosed(RTC::Room* room) = 0;
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
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		const RTC::RtpCapabilities& GetCapabilities() const;

	private:
		RTC::Peer* GetPeerFromRequest(Channel::Request* request, uint32_t* peerId = nullptr) const;
		void SetCapabilities(std::vector<RTC::RtpCodecParameters>& mediaCodecs);
		void AddRtpSenderForRtpReceiver(RTC::Peer* senderPeer, const RTC::RtpReceiver* rtpReceiver);

		/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		void OnPeerClosed(const RTC::Peer* peer) override;
		void OnPeerCapabilities(RTC::Peer* peer, RTC::RtpCapabilities* capabilities) override;
		void OnPeerRtpReceiverParameters(const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) override;
		void OnPeerRtpReceiverClosed(const RTC::Peer* peer, const RTC::RtpReceiver* rtpReceiver) override;
		void OnPeerRtpSenderClosed(const RTC::Peer* peer, RTC::RtpSender* rtpSender) override;
		void OnPeerRtpPacket(
		    const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) override;
		void OnPeerRtcpReceiverReport(
		    const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::ReceiverReport* report) override;
		void OnPeerRtcpFeedback(
		    const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackPsPacket* packet) override;
		void OnPeerRtcpFeedback(
		    const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackRtpPacket* packet) override;
		void OnPeerRtcpSenderReport(
		    const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RTCP::SenderReport* report) override;
		void OnFullFrameRequired(RTC::Peer* peer, RTC::RtpSender* rtpSender) override;

	public:
		// Passed by argument.
		uint32_t roomId{ 0 };

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		Channel::Notifier* notifier{ nullptr };
		// Others.
		RTC::RtpCapabilities capabilities;
		std::unordered_map<uint32_t, RTC::Peer*> peers;
		std::unordered_map<const RTC::RtpReceiver*, std::unordered_set<RTC::RtpSender*>> mapRtpReceiverRtpSenders;
		std::unordered_map<const RTC::RtpSender*, const RTC::RtpReceiver*> mapRtpSenderRtpReceiver;
	};

	/* Inline static methods. */

	inline const RTC::RtpCapabilities& Room::GetCapabilities() const
	{
		return this->capabilities;
	}
} // namespace RTC

#endif
