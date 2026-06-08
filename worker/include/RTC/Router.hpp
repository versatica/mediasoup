#ifndef MS_RTC_ROUTER_HPP
#define MS_RTC_ROUTER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/DataConsumer.hpp"
#include "RTC/DataProducer.hpp"
#include "RTC/Producer.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RtpObserver.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/Transport.hpp"
#include "RTC/WebRtcServer.hpp"
#include "SharedInterface.hpp"
#include <ankerl/unordered_dense.h>
#include <string>
#include <vector>

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
		explicit Router(SharedInterface* shared, const std::string& id, Listener* listener);
		~Router() override;

	public:
		flatbuffers::Offset<FBS::Router::DumpResponse> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const;

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

	private:
		RTC::Transport* AssertAndGetTransportById(const std::string& transportId) const;
		RTC::RtpObserver* AssertAndGetRtpObserverById(const std::string& rtpObserverId) const;
		void CheckNoTransport(const std::string& transportId) const;
		void CheckNoRtpObserver(const std::string& rtpObserverId) const;

		/* Pure virtual methods inherited from RTC::Transport::Listener. */
	public:
		void OnTransportNewProducer(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerClosed(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerPaused(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerResumed(RTC::Transport* transport, RTC::Producer* producer) override;
		void OnTransportProducerNewRtpStream(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  RTC::RTP::RtpStreamRecv* rtpStream,
		  uint32_t mappedSsrc) override;
		void OnTransportProducerRtpStreamScore(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  RTC::RTP::RtpStreamRecv* rtpStream,
		  uint8_t score,
		  uint8_t previousScore) override;
		void OnTransportProducerRtcpSenderReport(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  RTC::RTP::RtpStreamRecv* rtpStream,
		  bool first) override;
		void OnTransportProducerRtpPacketReceived(
		  RTC::Transport* transport, RTC::Producer* producer, RTC::RTP::Packet* packet) override;
		void OnTransportNeedWorstRemoteFractionLost(
		  RTC::Transport* transport,
		  RTC::Producer* producer,
		  uint32_t mappedSsrc,
		  uint8_t& worstRemoteFractionLost) override;
		void OnTransportNewConsumer(
		  RTC::Transport* transport, RTC::Consumer* consumer, const std::string& producerId) override;
		void OnTransportConsumerClosed(RTC::Transport* transport, RTC::Consumer* consumer) override;
		void OnTransportConsumerProducerClosed(RTC::Transport* transport, RTC::Consumer* consumer) override;
		void OnTransportConsumerKeyFrameRequested(
		  RTC::Transport* transport, RTC::Consumer* consumer, uint32_t mappedSsrc) override;
		void OnTransportNewDataProducer(RTC::Transport* transport, RTC::DataProducer* dataProducer) override;
		void OnTransportDataProducerClosed(RTC::Transport* transport, RTC::DataProducer* dataProducer) override;
		void OnTransportDataProducerPaused(RTC::Transport* transport, RTC::DataProducer* dataProducer) override;
		void OnTransportDataProducerResumed(
		  RTC::Transport* transport, RTC::DataProducer* dataProducer) override;
		void OnTransportDataProducerMessageReceived(
		  RTC::Transport* transport,
		  RTC::DataProducer* dataProducer,
		  RTC::SCTP::Message message,
		  std::vector<uint16_t>& subchannels,
		  std::optional<uint16_t> requiredSubchannel) override;
		void OnTransportNewDataConsumer(
		  RTC::Transport* transport, RTC::DataConsumer* dataConsumer, std::string& dataProducerId) override;
		void OnTransportDataConsumerClosed(RTC::Transport* transport, RTC::DataConsumer* dataConsumer) override;
		void OnTransportDataConsumerDataProducerClosed(
		  RTC::Transport* transport, RTC::DataConsumer* dataConsumer) override;
		void OnTransportListenServerClosed(RTC::Transport* transport) override;

		/* Pure virtual methods inherited from RTC::RtpObserver::Listener. */
	public:
		RTC::Producer* RtpObserverGetProducer(RTC::RtpObserver* rtpObserver, const std::string& id) override;
		void OnRtpObserverAddProducer(RTC::RtpObserver* rtpObserver, RTC::Producer* producer) override;
		void OnRtpObserverRemoveProducer(RTC::RtpObserver* rtpObserver, RTC::Producer* producer) override;

	public:
		// Passed by argument.
		std::string id;

	private:
		// Passed by argument.
		SharedInterface* shared{ nullptr };
		Listener* listener{ nullptr };
		// Allocated by this.
		ankerl::unordered_dense::map<std::string, RTC::Transport*> mapTransports;
		ankerl::unordered_dense::map<std::string, RTC::RtpObserver*> mapRtpObservers;
		// Others.
		ankerl::unordered_dense::map<RTC::Producer*, ankerl::unordered_dense::set<RTC::Consumer*>>
		  mapProducerConsumers;
		ankerl::unordered_dense::map<RTC::Consumer*, RTC::Producer*> mapConsumerProducer;
		ankerl::unordered_dense::map<RTC::Producer*, ankerl::unordered_dense::set<RTC::RtpObserver*>>
		  mapProducerRtpObservers;
		ankerl::unordered_dense::map<std::string, RTC::Producer*> mapProducers;
		ankerl::unordered_dense::map<RTC::DataProducer*, ankerl::unordered_dense::set<RTC::DataConsumer*>>
		  mapDataProducerDataConsumers;
		ankerl::unordered_dense::map<RTC::DataConsumer*, RTC::DataProducer*> mapDataConsumerDataProducer;
		ankerl::unordered_dense::map<std::string, RTC::DataProducer*> mapDataProducers;
	};
} // namespace RTC

#endif
