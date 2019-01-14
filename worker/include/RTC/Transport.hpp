#ifndef MS_RTC_TRANSPORT_HPP
#define MS_RTC_TRANSPORT_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTCP/CompoundPacket.hpp"
#include "RTC/RTCP/Packet.hpp"
#include "RTC/RTCP/ReceiverReport.hpp"
#include "RTC/RtpHeaderExtensionIds.hpp"
#include "RTC/RtpListener.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/TransportTuple.hpp"
#include "handles/Timer.hpp"
#include <string>
#include <unordered_map>

using json = nlohmann::json;

namespace RTC
{
	class Transport : public RTC::Producer::Listener,
	                  public RTC::Consumer::Listener,
	                  public Timer::Listener
	{
	public:
		class Listener
		{
		public:
			virtual void OnTransportNewProducer(RTC::Transport* transport, RTC::Producer* producer) = 0;
			virtual void OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer) = 0;
			virtual void OnTransportProducerPaused(RTC::Transport* transport, RTC::Producer* producer) = 0;
			virtual void OnTransportProducerResumed(RTC::Transport* transport, RTC::Producer* producer) = 0;
			virtual void OnTransportProducerStreamEnabled(
			  RTC::Transport* transport,
			  RTC::Producer* producer,
			  const RTC::RtpStream* rtpStream,
			  uint32_t mappedSsrc) = 0;
			virtual void OnTransportProducerStreamDisabled(
			  RTC::Transport* transport,
			  RTC::Producer* producer,
			  const RTC::RtpStream* rtpStream,
			  uint32_t mappedSsrc) = 0;
			virtual void OnTransportProducerRtpPacketReceived(
			  RTC::Transport* transport, RTC::Producer* producer, RTC::RtpPacket* packet) = 0;
			virtual const RTC::Producer* OnTransportGetProducer(
			  RTC::Transport* transport, std::string& producerId) = 0;
			virtual void OnTransportNewConsumer(
			  RTC::Transport* transport, RTC::Consumer* consumer, const RTC::Producer* producer) = 0;
			virtual void OnTransportConsumerClosed(RTC::Transport* transport, RTC::Consumer* consumer) = 0;
			virtual void OnTransportConsumerKeyFrameRequested(
			  RTC::Transport* transport, RTC::Consumer* consumer, uint32_t ssrc) = 0;
		};

	public:
		Transport(std::string id, Listener* listener);
		virtual ~Transport();

	public:
		// Subclasses must also invoke the parent Close().
		virtual void Close();
		void FillJson(json& jsonObject) const      = 0;
		void FillJsonStats(json& jsonObject) const = 0;
		// Subclasses must implement this method and call the parent's one to
		// handle common requests.
		virtual void HandleRequest(Channel::Request* request);

	protected:
		void SetNewProducerIdFromRequest(Channel::Request* request, std::string& producerId) const;
		RTC::Producer* GetProducerFromRequest(Channel::Request* request) const;
		void SetNewConsumerIdFromRequest(Channel::Request* request, std::string& consumerId) const;
		RTC::Consumer* GetConsumerFromRequest(Channel::Request* request) const;
		void ReceiveRtcpPacket(RTC::RTCP::Packet* packet);

	private:
		RTC::Consumer* GetStartedConsumer(uint32_t ssrc) const;
		void SendRtcp(uint64_t now);
		virtual bool IsConnected() const                                       = 0;
		virtual void SendRtpPacket(RTC::RtpPacket* packet)                     = 0;
		virtual void SendRtcpPacket(RTC::RTCP::Packet* packet)                 = 0;
		virtual void SendRtcpCompoundPacket(RTC::RTCP::CompoundPacket* packet) = 0;

		/* Pure virtual methods inherited from RTC::Producer::Listener. */
	public:
		void OnProducerPaused(RTC::Producer* producer) override;
		void OnProducerResumed(RTC::Producer* producer) override;
		void OnProducerStreamEnabled(
		  RTC::Producer* producer, const RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void OnProducerStreamDisabled(
		  RTC::Producer* producer, const RTC::RtpStream* rtpStream, uint32_t mappedSsrc) override;
		void OnProducerRtpPacketReceived(RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnProducerSendRtcpPacket(RTC::Producer* producer, RTC::RTCP::Packet* packet) override;

		/* Pure virtual methods inherited from RTC::Consumer::Listener. */
	public:
		void OnConsumerSendRtpPacket(RTC::Producer* consumer, RTC::Packet* packet) override;
		void OnConsumerKeyFrameRequired(RTC::Consumer* consumer) override;

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	public:
		// Passed by argument.
		const std::string id;

	protected:
		// Passed by argument.
		Listener* listener{ nullptr };
		// Allocated by this.
		std::unordered_map<std::string, RTC::Producer*> mapProducers;
		std::unordered_map<std::string, RTC::Consumer*> mapConsumers;
		Timer* rtcpTimer{ nullptr };
		// Others.
		RtpListener rtpListener;
		struct RTC::RtpHeaderExtensionIds rtpHeaderExtensionIds;
		uint32_t availableIncomingBitrate{ 0 };
		uint32_t availableOutgoingBitrate{ 0 };
		uint32_t maxIncomingBitrate{ 0 };
	};
} // namespace RTC

#endif
