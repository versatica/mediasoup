#ifndef MS_RTC_PEER_HPP
#define MS_RTC_PEER_HPP

#include "common.hpp"
#include "RTC/Transport.hpp"
#include "RTC/RtpReceiver.hpp"
#include "RTC/RtpSender.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "Channel/Request.hpp"
#include "Channel/Notifier.hpp"
#include "handles/Timer.hpp"
#include <string>
#include <unordered_map>
#include <vector>
#include <json/json.h>

namespace RTC
{
	class Peer :
		public RTC::Transport::Listener,
		public RTC::RtpReceiver::Listener,
		public RTC::RtpSender::Listener,
		public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void onPeerClosed(RTC::Peer* peer) = 0;
			virtual void onPeerCapabilities(RTC::Peer* peer, RTC::RtpCapabilities* capabilities) = 0;
			virtual void onPeerRtpReceiverParameters(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) = 0;
			virtual void onPeerRtpReceiverClosed(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) = 0;
			virtual void onPeerRtpSenderClosed(RTC::Peer* peer, RTC::RtpSender* rtpSender) = 0;
			virtual void onPeerRtpPacket(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) = 0;
			virtual void onPeerRtcpReceiverReport(RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::ReceiverReport* report) = 0;
			virtual void onPeerRtcpFeedback(RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackPsPacket* packet) = 0;
			virtual void onPeerRtcpFeedback(RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackRtpPacket* packet) = 0;
			virtual void onPeerRtcpSenderReport(RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RTCP::SenderReport* report) = 0;
		};

	private:
		static uint8_t rtcpBuffer[];

	public:
		Peer(Listener* listener, Channel::Notifier* notifier, uint32_t peerId, std::string& peerName);

	private:
		virtual ~Peer();

	public:
		void Destroy();
		Json::Value toJson() const;
		void HandleRequest(Channel::Request* request);
		bool HasCapabilities() const;
		std::vector<RTC::RtpReceiver*> GetRtpReceivers() const;
		std::vector<RTC::RtpSender*> GetRtpSenders() const;
		const std::unordered_map<uint32_t, RTC::Transport*>& GetTransports() const;
		/**
		 * Add a new RtpSender to the Peer.
		 * @param rtpSender     Instance of RtpSender.
		 * @param peerName      Name of the receiver Peer.
		 */
		void AddRtpSender(RTC::RtpSender* rtpSender, std::string& peerName, RTC::RtpParameters* rtpParameters);
		RTC::RtpSender* GetRtpSender(uint32_t ssrc) const;
		void SendRtcp(uint64_t now);

	private:
		RTC::Transport* GetTransportFromRequest(Channel::Request* request, uint32_t* transportId = nullptr) const;
		RTC::RtpReceiver* GetRtpReceiverFromRequest(Channel::Request* request, uint32_t* rtpReceiverId = nullptr) const;
		RTC::RtpSender* GetRtpSenderFromRequest(Channel::Request* request, uint32_t* rtpSenderId = nullptr) const;

	/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		virtual void onTransportClosed(RTC::Transport* transport) override;
		virtual void onTransportRtcpPacket(RTC::Transport* transport, RTC::RTCP::Packet* packet) override;

	/* Pure virtual methods inherited from RTC::RtpReceiver::Listener. */
	public:
		virtual void onRtpReceiverParameters(RTC::RtpReceiver* rtpReceiver) override;
		virtual void onRtpReceiverParametersDone(RTC::RtpReceiver* rtpReceiver) override;
		virtual void onRtpPacket(RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) override;
		virtual void onRtpReceiverClosed(RTC::RtpReceiver* rtpReceiver) override;

	/* Pure virtual methods inherited from RTC::RtpSender::Listener. */
	public:
		virtual void onRtpSenderClosed(RTC::RtpSender* rtpSender) override;

	/* Pure virtual methods inherited from Timer::Listener. */
	public:
		virtual void onTimer(Timer* timer) override;

	public:
		// Passed by argument.
		uint32_t peerId;
		std::string peerName;

	private:
		// Passed by argument.
		Listener* listener = nullptr;
		Channel::Notifier* notifier = nullptr;
		// Others.
		Timer* timer = nullptr;
		bool hasCapabilities = false;
		RTC::RtpCapabilities capabilities;
		std::unordered_map<uint32_t, RTC::Transport*> transports;
		std::unordered_map<uint32_t, RTC::RtpReceiver*> rtpReceivers;
		std::unordered_map<uint32_t, RTC::RtpSender*> rtpSenders;
	};

	/* Inline methods. */

	inline
	bool Peer::HasCapabilities() const
	{
		return this->hasCapabilities;
	}

	inline
	std::vector<RTC::RtpReceiver*> Peer::GetRtpReceivers() const
	{
		std::vector<RTC::RtpReceiver*> rtpReceivers;

		for (auto it = this->rtpReceivers.begin(); it != this->rtpReceivers.end(); ++it)
		{
			rtpReceivers.push_back(it->second);
		}

		return rtpReceivers;
	}

	inline
	std::vector<RTC::RtpSender*> Peer::GetRtpSenders() const
	{
		std::vector<RTC::RtpSender*> rtpSenders;

		for (auto it = this->rtpSenders.begin(); it != this->rtpSenders.end(); ++it)
		{
			rtpSenders.push_back(it->second);
		}

		return rtpSenders;
	}

	inline
	const std::unordered_map<uint32_t, RTC::Transport*>& Peer::GetTransports() const
	{
		return this->transports;
	}
}

#endif
