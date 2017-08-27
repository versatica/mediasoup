#ifndef MS_RTC_PEER_HPP
#define MS_RTC_PEER_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/Transport.hpp"
#include "handles/Timer.hpp"
#include <json/json.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace RTC
{
	class Peer : public RTC::Transport::Listener,
	             public RTC::Producer::Listener,
	             public RTC::Consumer::Listener,
	             public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnPeerClosed(const RTC::Peer* peer)                                        = 0;
			virtual void OnPeerProducerRtpParameters(const RTC::Peer* peer, RTC::Producer* producer)   = 0;
			virtual void OnPeerProducerClosed(const RTC::Peer* peer, const RTC::Producer* producer) = 0;
			virtual void OnPeerConsumerClosed(const RTC::Peer* peer, RTC::Consumer* consumer)       = 0;
			virtual void OnPeerRtpPacket(
			    const RTC::Peer* peer, RTC::Producer* producer, RTC::RtpPacket* packet) = 0;
			virtual void OnPeerRtcpReceiverReport(
			    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::ReceiverReport* report) = 0;
			virtual void OnPeerRtcpFeedback(
			    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::FeedbackPsPacket* packet) = 0;
			virtual void OnPeerRtcpFeedback(
			    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::FeedbackRtpPacket* packet) = 0;
			virtual void OnPeerRtcpSenderReport(
			    const RTC::Peer* peer, RTC::Producer* producer, RTC::RTCP::SenderReport* report) = 0;
			virtual void OnFullFrameRequired(RTC::Peer* peer, RTC::Consumer* consumer)           = 0;
		};

	public:
		Peer(Listener* listener, Channel::Notifier* notifier, uint32_t peerId, std::string& peerName);

	private:
		~Peer() override;

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);
		std::vector<RTC::Producer*> GetProducers() const;
		std::vector<RTC::Consumer*> GetConsumers() const;
		const std::unordered_map<uint32_t, RTC::Transport*>& GetTransports() const;
		/**
		 * Add a new Consumer to the Peer.
		 * @param consumer - Instance of Consumer.
		 * @param peerName - Name of the receiver Peer.
		 */
		void AddConsumer(
		    RTC::Consumer* consumer, RTC::RtpParameters* rtpParameters, uint32_t associatedProducerId);
		RTC::Consumer* GetConsumer(uint32_t ssrc) const;
		void SendRtcp(uint64_t now);

	private:
		RTC::Transport* GetTransportFromRequest(
		    Channel::Request* request, uint32_t* transportId = nullptr) const;
		RTC::Producer* GetProducerFromRequest(Channel::Request* request, uint32_t* producerId = nullptr) const;
		RTC::Consumer* GetConsumerFromRequest(Channel::Request* request, uint32_t* consumerId = nullptr) const;

		/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		void OnTransportConnected(RTC::Transport* transport) override;
		void OnTransportClosed(RTC::Transport* transport) override;
		void OnTransportRtcpPacket(RTC::Transport* transport, RTC::RTCP::Packet* packet) override;
		void OnTransportFullFrameRequired(RTC::Transport* transport) override;

		/* Pure virtual methods inherited from RTC::Producer::Listener. */
	public:
		void OnProducerRtpParameters(RTC::Producer* producer) override;
		void OnRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnProducerClosed(const RTC::Producer* producer) override;

		/* Pure virtual methods inherited from RTC::Consumer::Listener. */
	public:
		void OnConsumerClosed(RTC::Consumer* consumer) override;
		void OnConsumerFullFrameRequired(RTC::Consumer* consumer) override;

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
		std::unordered_map<uint32_t, RTC::Transport*> transports;
		std::unordered_map<uint32_t, RTC::Producer*> producers;
		std::unordered_map<uint32_t, RTC::Consumer*> consumers;
	};

	/* Inline methods. */

	inline std::vector<RTC::Producer*> Peer::GetProducers() const
	{
		std::vector<RTC::Producer*> producers;

		for (const auto& producer : this->producers)
		{
			producers.push_back(producer.second);
		}

		return producers;
	}

	inline std::vector<RTC::Consumer*> Peer::GetConsumers() const
	{
		std::vector<RTC::Consumer*> consumers;

		for (const auto& consumer : this->consumers)
		{
			consumers.push_back(consumer.second);
		}

		return consumers;
	}

	inline const std::unordered_map<uint32_t, RTC::Transport*>& Peer::GetTransports() const
	{
		return this->transports;
	}
} // namespace RTC

#endif
