#ifndef MS_RTC_ROUTER_HPP
#define MS_RTC_ROUTER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/ConsumerListener.hpp"
#include "RTC/Producer.hpp"
#include "RTC/ProducerListener.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/Transport.hpp"
#include <string>
#include <unordered_map>
#include <set>

using json = nlohmann::json;

namespace RTC
{
	class Router : public RTC::Transport::Listener,
	               public RTC::ProducerListener,
	               public RTC::ConsumerListener
	{
	public:
		Router(const std::string& routerId);
		virtual ~Router();

	public:
		void FillJson(json& jsonObject) const;
		void HandleRequest(Channel::Request* request);

	private:
		void SetNewTransportIdFromRequest(Channel::Request* request, std::string& transportId) const;
		RTC::Transport* GetTransportFromRequest(Channel::Request* request) const;
		void SetNewProducerIdFromRequest(Channel::Request* request, std::string& producerId) const;
		RTC::Producer* GetProducerFromRequest(Channel::Request* request) const;
		void SetNewConsumerIdFromRequest(Channel::Request* request, std::string& consumerId) const;
		RTC::Consumer* GetConsumerFromRequest(Channel::Request* request) const;

		/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		void OnTransportClosed(RTC::Transport* transport) override;

		/* Pure virtual methods inherited from RTC::ProducerListener. */
	public:
		void OnProducerClosed(RTC::Producer* producer) override;
		void OnProducerPaused(RTC::Producer* producer) override;
		void OnProducerResumed(RTC::Producer* producer) override;
		void OnProducerRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnProducerStreamEnabled(
		  RTC::Producer* producer,
		  const RTC::RtpStream* rtpStream,
		  uint32_t translatedSsrc) override;
		void OnProducerStreamDisabled(
		  RTC::Producer* producer,
		  const RTC::RtpStream* rtpStream,
		  uint32_t translatedSsrc) override;

		/* Pure virtual methods inherited from RTC::ConsumerListener. */
	public:
		void OnConsumerClosed(RTC::Consumer* consumer) override;
		void OnConsumerKeyFrameRequired(RTC::Consumer* consumer) override;

	public:
		// Passed by argument.
		std::string routerId;

	private:
		// Others.
		std::unordered_map<std::string, RTC::Transport*> transports;
		std::unordered_map<std::string, RTC::Producer*> producers;
		std::unordered_map<std::string, RTC::Consumer*> consumers;
		std::unordered_map<const RTC::Producer*, std::set<RTC::Consumer*>> mapProducerConsumers;
		std::unordered_map<const RTC::Consumer*, RTC::Producer*> mapConsumerProducer;
	};
} // namespace RTC

#endif
