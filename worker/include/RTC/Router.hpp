#ifndef MS_RTC_ROUTER_HPP
#define MS_RTC_ROUTER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "PayloadChannel/PayloadChannelNotification.hpp"
#include "PayloadChannel/PayloadChannelRequest.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/DataConsumer.hpp"
#include "RTC/DataProducer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RtpObserver.hpp"
#include "RTC/RtpPacket.hpp"
#include "RTC/RtpStream.hpp"
#include "RTC/Shared.hpp"
#include "RTC/Transport.hpp"
#include "RTC/WebRtcServer.hpp"
#include <absl/container/flat_hash_map.h>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

using json = nlohmann::json;

namespace RTC
{
	class Router : public RTC::Transport::Listener,
	               public RTC::RtpObserver::Listener,
	               public Channel::ChannelSocket::RequestHandler
	{
	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual RTC::WebRtcServer* OnRouterNeedWebRtcServer(
			  RTC::Router* router, std::string& webRtcServerId) = 0;
		};

	public:
		explicit Router(RTC::Shared* shared, const std::string& id, Listener* listener);
		virtual ~Router();

	public:
		void FillJson(json& jsonObject) const;

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

	private:
		void SetNewTransportIdFromData(json& data, std::string& transportId) const;
		RTC::Transport* GetTransportFromData(json& data) const;
		void SetNewRtpObserverIdFromData(json& data, std::string& rtpObserverId) const;
		RTC::RtpObserver* GetRtpObserverFromData(json& data) const;

		/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		void OnTransportNewProducer(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerPaused(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerResumed(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerNewRtpStream(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  RTC::RtpStream* rtpStream,
		  uint32_t mappedSsrc) override;
		void OnTransportProducerRtpStreamScore(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  RTC::RtpStream* rtpStream,
		  uint8_t score,
		  uint8_t previousScore) override;
		void OnTransportProducerRtcpSenderReport(
		  RTC::Transport* transport, RTC::Producer* producer, RTC::RtpStream* rtpStream, bool first) override;
		void OnTransportProducerRtpPacketReceived(
		  RTC::Transport* transport, RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void OnTransportNeedWorstRemoteFractionLost(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  uint32_t mappedSsrc,
		  uint8_t& worstRemoteFractionLost) override;
		void OnTransportNewConsumer(
		  RTC::Transport* transport, RTC::Consumer* consumer, std::string& producerId) override;
		void OnTransportConsumerClosed(RTC::Transport* transport, RTC::Consumer* consumer) override;
		void OnTransportConsumerProducerClosed(RTC::Transport* transport, RTC::Consumer* consumer) override;
		void OnTransportConsumerKeyFrameRequested(
		  RTC::Transport* transport, RTC::Consumer* consumer, uint32_t mappedSsrc) override;
		void OnTransportNewDataProducer(RTC::Transport* transport, RTC::DataProducer* dataProducer) override;
		void OnTransportDataProducerClosed(RTC::Transport* transport, RTC::DataProducer* dataProducer) override;
		void OnTransportDataProducerMessageReceived(
		  RTC::Transport* transport,
		  RTC::DataProducer* dataProducer,
		  uint32_t ppid,
		  const uint8_t* msg,
		  size_t len) override;
		void OnTransportNewDataConsumer(
		  RTC::Transport* transport, RTC::DataConsumer* dataConsumer, std::string& dataProducerId) override;
		void OnTransportDataConsumerClosed(RTC::Transport* transport, RTC::DataConsumer* dataConsumer) override;
		void OnTransportDataConsumerDataProducerClosed(
		  RTC::Transport* transport, RTC::DataConsumer* dataConsumer) override;
		void OnTransportListenServerClosed(RTC::Transport* transport) override;

		/* Pure virtual methods inherited from RTC::RtpObserver::Listener. */
	public:
		RTC::Producer* RtpObserverGetProducer(RTC::RtpObserver*, const std::string& id) override;
		void OnRtpObserverAddProducer(RTC::RtpObserver* rtpObserver, RTC::Producer* producer) override;
		void OnRtpObserverRemoveProducer(RTC::RtpObserver* rtpObserver, RTC::Producer* producer) override;

	public:
		// Passed by argument.
		const std::string id;

	private:
		// Passed by argument.
		RTC::Shared* shared{ nullptr };
		Listener* listener{ nullptr };
		// Allocated by this.
		absl::flat_hash_map<std::string, RTC::Transport*> mapTransports;
		absl::flat_hash_map<std::string, RTC::RtpObserver*> mapRtpObservers;
		// Others.
		absl::flat_hash_map<RTC::Producer*, absl::flat_hash_set<RTC::Consumer*>> mapProducerConsumers;
		absl::flat_hash_map<RTC::Consumer*, RTC::Producer*> mapConsumerProducer;
		absl::flat_hash_map<RTC::Producer*, absl::flat_hash_set<RTC::RtpObserver*>> mapProducerRtpObservers;
		absl::flat_hash_map<std::string, RTC::Producer*> mapProducers;
		absl::flat_hash_map<RTC::DataProducer*, absl::flat_hash_set<RTC::DataConsumer*>>
		  mapDataProducerDataConsumers;
		absl::flat_hash_map<RTC::DataConsumer*, RTC::DataProducer*> mapDataConsumerDataProducer;
		absl::flat_hash_map<std::string, RTC::DataProducer*> mapDataProducers;
	};
} // namespace RTC

#endif
