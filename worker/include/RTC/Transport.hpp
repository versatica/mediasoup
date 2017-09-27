#ifndef MS_RTC_TRANSPORT_HPP
#define MS_RTC_TRANSPORT_HPP

#include "common.hpp"
#include "Channel/Notifier.hpp"
#include "RTC/ConsumerListener.hpp"
#include "RTC/ProducerListener.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpListener.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/TransportTuple.hpp"
#include "RTC/UdpSocket.hpp"
#include "handles/Timer.hpp"
#include <json/json.h>
#include <unordered_set>

namespace RTC
{
	// Avoid cyclic #include problem by declaring classes instead of including
	// the corresponding header files.
	class Producer;
	class Consumer;

	class Transport : public RTC::ProducerListener,
	                  public RTC::ConsumerListener,
	                  public Timer::Listener,
	                  public RTC::UdpSocket::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnTransportClosed(RTC::Transport* transport) = 0;
			virtual void OnTransportReceiveRtcpFeedback(
			  RTC::Transport* transport, RTC::Consumer* consumer, RTC::RTCP::FeedbackPsPacket* packet) = 0;
		};

	public:
		// RTP header extension ids that must be shared by all the Producers using
		// the same Transport.
		// NOTE: These ids are the original ids in the RTP packet (before the Producer
		// maps them to the corresponding ids in the room).
		struct HeaderExtensionIds
		{
			uint8_t absSendTime{ 0 }; // 0 means no abs-send-time id.
			uint8_t rid{ 0 };         // 0 means no RID id.
		};

	public:
		// Mirroring options.
		// Determines which incoming traffic to mirror and where.
		struct MirroringOptions
		{
			std::string remoteIP;
			uint16_t remotePort;
			bool rtp{ true };
			bool rtcp{ true };
		};

	public:
		Transport(Listener* listener, Channel::Notifier* notifier, uint32_t transportId);

	protected:
		virtual ~Transport();

	public:
		void Destroy();
		virtual Json::Value ToJson() const = 0;
		void HandleProducer(RTC::Producer* producer);
		void HandleConsumer(RTC::Consumer* consumer);
		virtual void SendRtpPacket(RTC::RtpPacket* packet)     = 0;
		virtual void SendRtcpPacket(RTC::RTCP::Packet* packet) = 0;
		void StartMirroring(MirroringOptions& options);
		void StopMirroring();

	protected:
		void HandleRtcpPacket(RTC::RTCP::Packet* packet);

	private:
		virtual bool IsConnected() const = 0;
		void SendRtcp(uint64_t now);
		virtual void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) = 0;
		RTC::Consumer* GetConsumer(uint32_t ssrc) const;

		/* Pure virtual methods inherited from RTC::ProducerListener. */
	public:
		void OnProducerClosed(RTC::Producer* producer) override;
		void OnProducerRtpParametersUpdated(RTC::Producer* producer) override;
		void OnProducerPaused(RTC::Producer* producer) override;
		void OnProducerResumed(RTC::Producer* producer) override;
		void OnProducerRtpPacket(
		  RTC::Producer* producer,
		  RTC::RtpPacket* packet,
		  RTC::RtpEncodingParameters::Profile profile) override;
		void OnProducerProfileEnabled(
		  RTC::Producer* producer, RTC::RtpEncodingParameters::Profile profile) override;
		void OnProducerProfileDisabled(
		  RTC::Producer* producer, RTC::RtpEncodingParameters::Profile profile) override;

		/* Pure virtual methods inherited from RTC::ConsumerListener. */
	public:
		void OnConsumerClosed(RTC::Consumer* consumer) override;
		void OnConsumerKeyFrameRequired(RTC::Consumer* consumer) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

		/* Pure virtual methods inherited from UdpSocket::Listener. Mirroring. */
	public:
		void OnPacketRecv(
		  RTC::UdpSocket* socket, const uint8_t* data, size_t len, const struct sockaddr* remoteAddr) override;

	public:
		// Passed by argument.
		uint32_t transportId{ 0 };

	protected:
		// Passed by argument.
		Listener* listener{ nullptr };
		Channel::Notifier* notifier{ nullptr };
		// Allocated by this.
		Timer* rtcpTimer{ nullptr };
		// Allocated (Mirroring).
		RTC::UdpSocket* mirrorSocket{ nullptr };
		RTC::TransportTuple* mirrorTuple{ nullptr };
		// Others (Producers and Consumers).
		std::unordered_set<RTC::Producer*> producers;
		std::unordered_set<RTC::Consumer*> consumers;
		// Others (RtpListener).
		RtpListener rtpListener;
		struct HeaderExtensionIds headerExtensionIds;
		// Others (Mirroring).
		MirroringOptions mirroringOptions;
		struct sockaddr_storage mirrorAddrStorage
		{
		};
	};
} // namespace RTC

#endif
