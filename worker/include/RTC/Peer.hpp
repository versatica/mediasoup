#ifndef MS_RTC_PEER_HPP
#define MS_RTC_PEER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpReceiver.hpp"
#include "RTC/RtpSender.hpp"
#include "RTC/Transport.hpp"
#include "handles/Timer.hpp"
#include <json/json.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace RTC
{
	class Peer : public RTC::Transport::Listener,
	             public RTC::RtpReceiver::Listener,
	             public RTC::RtpSender::Listener,
	             public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnPeerClosed(const RTC::Peer* peer)                                     = 0;
			virtual void OnPeerCapabilities(RTC::Peer* peer, RTC::RtpCapabilities* capabilities) = 0;
			virtual void OnPeerRtpReceiverParameters(const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver) = 0;
			virtual void OnPeerRtpReceiverClosed(
			    const RTC::Peer* peer, const RTC::RtpReceiver* rtpReceiver)                      = 0;
			virtual void OnPeerRtpSenderClosed(const RTC::Peer* peer, RTC::RtpSender* rtpSender) = 0;
			virtual void OnPeerRtpPacket(
			    const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) = 0;
			virtual void OnPeerRtcpReceiverReport(
			    const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::ReceiverReport* report) = 0;
			virtual void OnPeerRtcpFeedback(
			    const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackPsPacket* packet) = 0;
			virtual void OnPeerRtcpFeedback(
			    const RTC::Peer* peer, RTC::RtpSender* rtpSender, RTC::RTCP::FeedbackRtpPacket* packet) = 0;
			virtual void OnPeerRtcpSenderReport(
			    const RTC::Peer* peer, RTC::RtpReceiver* rtpReceiver, RTC::RTCP::SenderReport* report) = 0;
			virtual void OnFullFrameRequired(RTC::Peer* peer, RTC::RtpSender* rtpSender) = 0;
		};

	public:
		Peer(Listener* listener, Channel::Notifier* notifier, uint32_t peerId, std::string& peerName);

	private:
		~Peer() override;

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		bool HasCapabilities() const;
		std::vector<RTC::RtpReceiver*> GetRtpReceivers() const;
		std::vector<RTC::RtpSender*> GetRtpSenders() const;
		const std::unordered_map<uint32_t, RTC::Transport*>& GetTransports() const;
		/**
		 * Add a new RtpSender to the Peer.
		 * @param rtpSender - Instance of RtpSender.
		 * @param peerName - Name of the receiver Peer.
		 */
		void AddRtpSender(
		    RTC::RtpSender* rtpSender, RTC::RtpParameters* rtpParameters, uint32_t associatedRtpReceiverId);
		RTC::RtpSender* GetRtpSender(uint32_t ssrc) const;
		void SendRtcp(uint64_t now);

	private:
		RTC::Transport* GetTransportFromRequest(
		    Channel::Request* request, uint32_t* transportId = nullptr) const;
		RTC::RtpReceiver* GetRtpReceiverFromRequest(
		    Channel::Request* request, uint32_t* rtpReceiverId = nullptr) const;
		RTC::RtpSender* GetRtpSenderFromRequest(
		    Channel::Request* request, uint32_t* rtpSenderId = nullptr) const;

		/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		void OnTransportConnected(RTC::Transport* transport) override;
		void OnTransportClosed(RTC::Transport* transport) override;
		void OnTransportRtcpPacket(RTC::Transport* transport, RTC::RTCP::Packet* packet) override;
		void OnTransportFullFrameRequired(RTC::Transport* transport) override;

		/* Pure virtual methods inherited from RTC::RtpReceiver::Listener. */
	public:
		void OnRtpReceiverParameters(RTC::RtpReceiver* rtpReceiver) override;
		void OnRtpReceiverParametersDone(RTC::RtpReceiver* rtpReceiver) override;
		void OnRtpPacket(RTC::RtpReceiver* rtpReceiver, RTC::RtpPacket* packet) override;
		void OnRtpReceiverClosed(const RTC::RtpReceiver* rtpReceiver) override;

		/* Pure virtual methods inherited from RTC::RtpSender::Listener. */
	public:
		void OnRtpSenderClosed(RTC::RtpSender* rtpSender) override;
		void OnRtpSenderFullFrameRequired(RTC::RtpSender* rtpSender) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Passed by argument.
		uint32_t peerId{ 0 };
		std::string peerName;

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		Channel::Notifier* notifier{ nullptr };
		// Others.
		Timer* timer{ nullptr };
		bool hasCapabilities{ false };
		RTC::RtpCapabilities capabilities;
		std::unordered_map<uint32_t, RTC::Transport*> transports;
		std::unordered_map<uint32_t, RTC::RtpReceiver*> rtpReceivers;
		std::unordered_map<uint32_t, RTC::RtpSender*> rtpSenders;
	};

	/* Inline methods. */

	inline bool Peer::HasCapabilities() const
	{
		return this->hasCapabilities;
	}

	inline std::vector<RTC::RtpReceiver*> Peer::GetRtpReceivers() const
	{
		std::vector<RTC::RtpReceiver*> rtpReceivers;

		for (const auto& rtpReceiver : this->rtpReceivers)
		{
			rtpReceivers.push_back(rtpReceiver.second);
		}

		return rtpReceivers;
	}

	inline std::vector<RTC::RtpSender*> Peer::GetRtpSenders() const
	{
		std::vector<RTC::RtpSender*> rtpSenders;

		for (const auto& rtpSender : this->rtpSenders)
		{
			rtpSenders.push_back(rtpSender.second);
		}

		return rtpSenders;
	}

	inline const std::unordered_map<uint32_t, RTC::Transport*>& Peer::GetTransports() const
	{
		return this->transports;
	}
} // namespace RTC

#endif
