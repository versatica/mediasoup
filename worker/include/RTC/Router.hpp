#ifndef MS_RTC_ROUTER_HPP
#define MS_RTC_ROUTER_HPP

#include "common.hpp"
#include "json.hpp"
#include "Channel/Request.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RtpObserver.hpp"
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
		explicit Router(const std::string& id);
		virtual ~Router();

	public:
		void FillJson(json& jsonObject) const;
		void HandleRequest(Channel::Request* request);

	private:
		void SetNewTransportIdFromRequest(Channel::Request* request, std::string& transportId) const;
		RTC::Transport* GetTransportFromRequest(Channel::Request* request) const;
		void SetNewRtpObserverIdFromRequest(Channel::Request* request, std::string& rtpObserverId) const;
		RTC::RtpObserver* GetRtpObserverFromRequest(Channel::Request* request) const;
		RTC::Producer* GetProducerFromRequest(Channel::Request* request) const;

		/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		void OnTransportNewProducer(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerPaused(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerResumed(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerNewRtpStream(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  RTC::RtpStreamRecv* rtpStream,
		  uint32_t mappedSsrc) override;
		void OnTransportProducerRtpPacketReceived(
		  RTC::Transport* transport, RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnTransportNeedWorstRemoteFractionLost(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  uint32_t mappedSsrc,
		  uint8_t& worstRemoteFractionLost) override;
		void OnTransportProducerRtpStreamScore(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  RTC::RtpStreamRecv* rtpStream,
		  uint8_t score) override;
		void OnTransportNewConsumer(
		  RTC::Transport* transport, RTC::Consumer* consumer, std::string& producerId) override;
		void OnTransportConsumerClosed(RTC::Transport* transport, RTC::Consumer* consumer) override;
		void OnTransportConsumerProducerClosed(RTC::Transport* transport, RTC::Consumer* consumer) override;
		void OnTransportConsumerKeyFrameRequested(
		  RTC::Transport* transport, RTC::Consumer* consumer, uint32_t mappedSsrc) override;

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Allocated by this.
		std::unordered_map<std::string, RTC::Transport*> mapTransports;
		std::unordered_map<std::string, RTC::RtpObserver*> mapRtpObservers;
		// Others.
		std::unordered_map<RTC::Producer*, std::unordered_set<RTC::Consumer*>> mapProducerConsumers;
		std::unordered_map<RTC::Consumer*, RTC::Producer*> mapConsumerProducer;
		std::unordered_map<RTC::Producer*, std::unordered_set<RTC::RtpObserver*>> mapProducerRtpObservers;
		std::unordered_map<std::string, RTC::Producer*> mapProducers;
	};
} // namespace RTC

#endif
