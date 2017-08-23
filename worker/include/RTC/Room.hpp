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
		void AddConsumerForProducer(RTC::Peer* senderPeer, const RTC::Producer* producer);

		/* Pure virtual methods inherited from RTC::Peer::Listener. */
	public:
		void OnPeerClosed(const RTC::Peer* peer) override;
		void OnPeerCapabilities(RTC::Peer* peer, RTC::RtpCapabilities* capabilities) override;
		void OnPeerProducerParameters(const RTC::Peer* peer, RTC::Producer* producer) override;
		void OnPeerProducerClosed(const RTC::Peer* peer, const RTC::Producer* producer) override;
		void OnPeerConsumerClosed(const RTC::Peer* peer, RTC::Consumer* consumer) override;
		void OnPeerRtpPacket(const RTC::Peer* peer, RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnPeerRtcpReceiverReport(
		    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::ReceiverReport* report) override;
		void OnPeerRtcpFeedback(
		    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::FeedbackPsPacket* packet) override;
		void OnPeerRtcpFeedback(
		    const RTC::Peer* peer, RTC::Consumer* consumer, RTC::RTCP::FeedbackRtpPacket* packet) override;
		void OnPeerRtcpSenderReport(
		    const RTC::Peer* peer, RTC::Producer* producer, RTC::RTCP::SenderReport* report) override;
		void OnFullFrameRequired(RTC::Peer* peer, RTC::Consumer* consumer) override;

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
		RTC::RtpCapabilities capabilities;
		std::unordered_map<uint8_t, RTC::RtpCodecParameters> mapPayloadRtxCodecParameters;
		std::unordered_map<uint32_t, RTC::Peer*> peers;
		std::unordered_map<const RTC::Producer*, std::unordered_set<RTC::Consumer*>> mapProducerConsumers;
		std::unordered_map<const RTC::Consumer*, const RTC::Producer*> mapConsumerProducer;
		std::unordered_map<RTC::Producer*, std::vector<int8_t>> mapProducerAudioLevels;
		bool audioLevelsEventEnabled{ false };
	};

	/* Inline static methods. */

	inline const RTC::RtpCapabilities& Room::GetCapabilities() const
	{
		return this->capabilities;
	}
} // namespace RTC

#endif
