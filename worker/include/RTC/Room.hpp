#ifndef MS_RTC_ROOM_HPP
#define MS_RTC_ROOM_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/Request.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Peer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTCP/Feedback.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RTCP/Sdes.hpp"
#include "RTC/RTCP/SenderReport.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/RtpPacket.hpp"
#include "handles/Timer.hpp"
#include <json/json.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace RTC
{
	class Room : public RTC::Peer::Listener, public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnRoomClosed(RTC::Room* room) = 0;
		};

	public:
		Room(Listener* listener, Channel::Notifier* notifier, uint32_t roomId);

	private:
		virtual ~Room();

	public:
		void Destroy();
		Json::Value ToJson() const;
		void HandleRequest(Channel::Request* request);

	private:
		RTC::Peer* GetPeerFromRequest(Channel::Request* request, uint32_t* peerId = nullptr) const;
		void AddConsumerForProducer(RTC::Peer* consumerPeer, const RTC::Producer* producer);

		/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		void OnPeerClosed(const RTC::Peer* peer) override;
		void OnPeerProducerClosed(const RTC::Peer* peer, const RTC::Producer* producer) override;
		void OnPeerProducerRtpParameters(const RTC::Peer* peer, RTC::Producer* producer) override;
		void OnPeerProducerPaused(const RTC::Peer* peer, const RTC::Producer* producer) override;
		void OnPeerProducerResumed(const RTC::Peer* peer, const RTC::Producer* producer) override;
		void OnPeerProducerRtpPacket(
		    const RTC::Peer* peer, RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnPeerProducerRtcpSenderReport(
		    const RTC::Peer* peer, RTC::Producer* producer, RTC::RTCP::SenderReport* report) override;
		void OnPeerConsumerClosed(const RTC::Peer* peer, RTC::Consumer* consumer) override;
		void OnPeerConsumerRtcpReceiverReport(
		    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::ReceiverReport* report) override;
		void OnPeerConsumerRtcpFeedback(
		    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::FeedbackPsPacket* packet) override;
		void OnPeerConsumerRtcpFeedback(
		    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::FeedbackRtpPacket* packet) override;
		void OnPeerConsumerFullFrameRequired(RTC::Peer* peer, RTC::Consumer* consumer) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Passed by argument.
		uint32_t roomId{ 0 };

	private:
		// Passed by argument.
		Listener* listener{ nullptr };
		Channel::Notifier* notifier{ nullptr };
		// Allocated by this.
		Timer* audioLevelsTimer{ nullptr };
		// Others.
		std::unordered_map<uint32_t, RTC::Peer*> peers;
		std::unordered_map<const RTC::Producer*, std::unordered_set<RTC::Consumer*>> mapProducerConsumers;
		std::unordered_map<const RTC::Consumer*, const RTC::Producer*> mapConsumerProducer;
		std::unordered_map<RTC::Producer*, std::vector<int8_t>> mapProducerAudioLevels;
		bool audioLevelsEventEnabled{ false };
	};
} // namespace RTC

#endif
