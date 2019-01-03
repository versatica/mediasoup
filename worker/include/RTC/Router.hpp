#ifndef MS_RTC_ROUTER_HPP
#define MS_RTC_ROUTER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/Transport.hpp"
#include <string>
#include <unordered_map>
#include <unordered_set>

using json = nlohmann::json;

namespace RTC
{
	class Router : public RTC::Transport::Listener
	{
	public:
		Router(const std::string& id);
		virtual ~Router();

	public:
		void FillJson(json& jsonObject) const;
		void HandleRequest(Channel::Request* request);

	private:
		void SetNewTransportIdFromRequest(Channel::Request* request, std::string& transportId) const;
		RTC::Transport* GetTransportFromRequest(Channel::Request* request) const;

		/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		void OnTransportClosed(RTC::Transport* transport) override;
		void OnTransportNewProducer(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerPaused(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerResumed(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerStreamEnabled(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  const RTC::RtpStream* rtpStream,
		  uint32_t translatedSsrc) override;
		void OnTransportProducerStreamDisabled(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  const RTC::RtpStream* rtpStream,
		  uint32_t translatedSsrc) override;
		void OnTransportProducerRtpPacket(
		  RTC::Transport* transport, RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnTransportNewConsumer(RTC::Transport* transport, RTC::Consumer* consumer) override;
		void OnTransportConsumerClosed(RTC::Transport* transport, RTC::Consumer* consumer) override;
		void OnTransportConsumerKeyFrameRequested(
		  RTC::Transport* transport, RTC::Consumer* consumer, uint32_t ssrc) override;

	public:
		// Passed by argument.
		std::string id;

	private:
		// Others.
		std::unordered_map<std::string, RTC::Transport*> transports;
		std::unordered_map<const RTC::Producer*, std::unordered_set<RTC::Consumer*>> mapProducerConsumers;
		std::unordered_map<const RTC::Consumer*, RTC::Producer*> mapConsumerProducer;
	};
} // namespace RTC

#endif
