#define MS_CLASS "RTC::Transport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Transport.hpp"
#include "FBS/sctpAssociation.h"
#include "FBS/transport.h"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "RTC/Consts.hpp"
#include "RTC/Consumer.hpp"
#include "RTC/RTCP/FeedbackPs.hpp"
#include "RTC/RTCP/FeedbackPsAfb.hpp"
#include "RTC/RTCP/FeedbackPsFir.hpp"
#include "RTC/RTCP/FeedbackPsRemb.hpp"
#include "RTC/RTCP/FeedbackRtpNack.hpp"
#include "RTC/RTCP/FeedbackRtpTransport.hpp"
#include "RTC/RTCP/XrDelaySinceLastRr.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "RTC/SCTP/association/Association.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "RTC/SCTP/public/SctpOptions.hpp"
#include "RTC/SubchannelsCodec.hpp"
#include "Utils.hpp"
#ifdef MS_RTC_LOGGER_RTP
#include "RTC/RtcLogger/RtpPacket.hpp"
#endif
#include <array>

namespace RTC
{
	/* Static. */

	// Highest bitrate the API may ask for (bps), which anything higher is brought
	// down to. A limit above what the bandwidth estimation deals in is not a
	// limit, and it also keeps `Types::BitrateInfinite` out of the estimators,
	// which reserve it to mean that there is no limit at all.
	static constexpr int64_t AbsoluteMaxBitrate{ RTC::Consts::BweMaxBitrate };
	// Lowest bitrate the API may ask for (bps). A limit below it is not a limit,
	// it is a way of sending nothing at all.
	static constexpr int64_t AbsoluteMinBitrate{ 30000 };
	// Bitrate the outgoing target starts at when the API does not say (bps).
	static constexpr int64_t DefaultInitialAvailableOutgoingBitrate{ 600000 };
#ifdef MS_RTC_LOGGER_SEND_BURST
	// How often the distribution of send bursts is printed.
	static constexpr int64_t SendBurstLogIntervalMs{ 5000 };
#endif

	/* Instance methods. */

	Transport::Transport(
	  SharedInterface* shared,
	  const std::string& id,
	  RTC::Transport::Listener* listener,
	  const FBS::Transport::Options* options,
	  bool requireSctpStateCookieAuthentication)
	  : id(id),
	    shared(shared),
	    listener(listener),
	    rtcpTimer(shared->CreateTimer(this, "transport-rtcp")),
	    enableBwe(options->enableBwe()),
	    direct(options->direct()),
	    recvRtpTransmission(shared, /*ignorePaddingOnlyPackets*/ false),
	    sendRtpTransmission(shared, /*ignorePaddingOnlyPackets*/ false),
	    recvRtxTransmission(shared, /*ignorePaddingOnlyPackets*/ false, 1000),
	    sendRtxTransmission(shared, /*ignorePaddingOnlyPackets*/ false, 1000),
	    sendProbationTransmission(shared, /*ignorePaddingOnlyPackets*/ false, 100),
	    rttCalculator(this, shared),
	    maxSendMessageSize(options->maxSendMessageSize()),
	    maxReceiveMessageSize(options->maxReceiveMessageSize())
#ifdef MS_RTC_LOGGER_SEND_BURST
	    ,
	    sendBurstLoggerTimer(shared->CreateTimer(this, "transport-send-burst"))
#endif
	{
		MS_TRACE();

#ifdef MS_RTC_LOGGER_SEND_BURST
		this->sendBurstLogger.transportId = this->id;

		this->sendBurstLoggerTimer->Start(SendBurstLogIntervalMs, SendBurstLogIntervalMs);
#endif

		// NOTE: The API gives unsigned 64 bits bitrates, so they are clamped here.
		if (this->enableBwe)
		{
			this->maxIncomingBitrate = AbsoluteMaxBitrate;
			this->minOutgoingBitrate = AbsoluteMinBitrate;
			this->maxOutgoingBitrate = AbsoluteMaxBitrate;

			if (auto maxIncomingBitrate = options->maxIncomingBitrate(); maxIncomingBitrate.has_value())
			{
				this->maxIncomingBitrate =
				  static_cast<int64_t>(std::min<uint64_t>(maxIncomingBitrate.value(), AbsoluteMaxBitrate));
			}

			if (auto minOutgoingBitrate = options->minOutgoingBitrate(); minOutgoingBitrate.has_value())
			{
				this->minOutgoingBitrate =
				  static_cast<int64_t>(std::min<uint64_t>(minOutgoingBitrate.value(), AbsoluteMaxBitrate));
			}

			if (auto maxOutgoingBitrate = options->maxOutgoingBitrate(); maxOutgoingBitrate.has_value())
			{
				this->maxOutgoingBitrate =
				  static_cast<int64_t>(std::min<uint64_t>(maxOutgoingBitrate.value(), AbsoluteMaxBitrate));
			}

			this->initialAvailableOutgoingBitrate = DefaultInitialAvailableOutgoingBitrate;

			if (
			  auto initialAvailableOutgoingBitrate = options->initialAvailableOutgoingBitrate();
			  initialAvailableOutgoingBitrate.has_value())
			{
				this->initialAvailableOutgoingBitrate = static_cast<int64_t>(
				  std::min<uint64_t>(initialAvailableOutgoingBitrate.value(), AbsoluteMaxBitrate));
			}

			MayFixBweBitrateLimits();
		}

		if (options->enableSctp())
		{
			if (this->direct)
			{
				MS_THROW_TYPE_ERROR("cannot enable SCTP in a direct Transport");
			}

			if (options->sctpMaxReceiverWindowBufferSize() < RTC::Consts::SctpReceiverWindowBufferMinSize)
			{
				MS_THROW_TYPE_ERROR(
				  "sctpMaxReceiverWindowBufferSize must be at least %zu",
				  RTC::Consts::SctpReceiverWindowBufferMinSize);
			}

			RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod
			  zeroChecksumAlternateErrorDetectionMethod{
				  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
			  };

			switch (options->sctpZeroChecksum())
			{
				case FBS::Transport::SctpZeroChecksum::SCTP_OVER_DTLS:
				{
					zeroChecksumAlternateErrorDetectionMethod =
					  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::SCTP_OVER_DTLS;

					break;
				}

				case FBS::Transport::SctpZeroChecksum::TRUSTED_NETWORK:
				{
					zeroChecksumAlternateErrorDetectionMethod =
					  RTC::SCTP::ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::TRUSTED_NETWORK;

					break;
				}

				default:;
			}

			const RTC::SCTP::SctpOptions sctpOptions = {
				.mtu                         = RTC::Consts::OutgoingSctpPacketMaxSize,
				.maxSendMessageSize          = this->maxSendMessageSize,
				.maxSendBufferSize           = options->sctpSendBufferSize(),
				.perStreamSendQueueLimit     = options->sctpPerStreamSendQueueLimit(),
				.maxReceiveMessageSize       = this->maxReceiveMessageSize,
				.maxReceiverWindowBufferSize = options->sctpMaxReceiverWindowBufferSize(),
				.defaultStreamBufferedAmountLowThreshold =
				  options->sctpDefaultStreamBufferedAmountLowThreshold(),
				.zeroChecksumAlternateErrorDetectionMethod = zeroChecksumAlternateErrorDetectionMethod,
				.requireAuthenticatedCookie                = requireSctpStateCookieAuthentication
			};

			this->sctpAssociation = std::make_unique<RTC::SCTP::Association>(
			  sctpOptions, this, this->shared, options->isDataChannel(), /*mayConnectOnReceivedSctpData*/ true);
		}
	}

	Transport::~Transport()
	{
		MS_TRACE();

		// The destructor must delete and clear everything silently.

		// Delete all Producers.
		for (auto& kv : this->mapProducers)
		{
			auto* producer = kv.second;

			delete producer;
		}
		this->mapProducers.clear();

		// Delete all Consumers.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			delete consumer;
		}
		this->mapConsumers.clear();
		this->mapSsrcConsumer.clear();
		this->mapRtxSsrcConsumer.clear();

		// Delete all DataProducers.
		for (auto& kv : this->mapDataProducers)
		{
			auto* dataProducer = kv.second;

			delete dataProducer;
		}
		this->mapDataProducers.clear();

		// Delete all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			delete dataConsumer;
		}
		this->mapDataConsumers.clear();
		this->mapSctpStreamIdDataConsumers.clear();

		// NOTE: We don't close `this->sctpAssociation` here since the
		// `SetDestroying()` method has already been called by the Transport
		// subclass and it closed the SCTP association.
		//
		// NOTE: We cannot do it here in the destructor because here we are no longer
		// the Transport subclass but Transport parent (this is how the destruction
		// chain works in C++).
	}

	void Transport::CloseProducersAndConsumers()
	{
		MS_TRACE();

		// This method is called by the Router and must notify him about all Producers
		// and Consumers that we are gonna close.
		//
		// The caller is supposed to delete this Transport instance after calling
		// this method.

		// Close all Producers.
		for (auto& kv : this->mapProducers)
		{
			auto* producer = kv.second;

			// Notify the listener.
			this->listener->OnTransportProducerClosed(this, producer);

			delete producer;
		}
		this->mapProducers.clear();

		// Delete all Consumers.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			// Notify the listener.
			this->listener->OnTransportConsumerClosed(this, consumer);

			delete consumer;
		}
		this->mapConsumers.clear();
		this->mapSsrcConsumer.clear();
		this->mapRtxSsrcConsumer.clear();

		// Delete all DataProducers.
		for (auto& kv : this->mapDataProducers)
		{
			auto* dataProducer = kv.second;

			// Notify the listener.
			this->listener->OnTransportDataProducerClosed(this, dataProducer);

			delete dataProducer;
		}
		this->mapDataProducers.clear();

		// Delete all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			// Notify the listener.
			this->listener->OnTransportDataConsumerClosed(this, dataConsumer);

			delete dataConsumer;
		}
		this->mapDataConsumers.clear();
		this->mapSctpStreamIdDataConsumers.clear();
	}

	void Transport::ListenServerClosed()
	{
		MS_TRACE();

		// Ask our parent Router to close/delete us.
		this->listener->OnTransportListenServerClosed(this);
	}

	flatbuffers::Offset<FBS::Transport::Dump> Transport::FillBuffer(
	  flatbuffers::FlatBufferBuilder& builder) const
	{
		MS_TRACE();

		// Add producerIds.
		std::vector<flatbuffers::Offset<flatbuffers::String>> producerIds;

		for (const auto& kv : this->mapProducers)
		{
			const auto& producerId = kv.first;

			producerIds.emplace_back(builder.CreateString(producerId));
		}

		// Add consumerIds.
		std::vector<flatbuffers::Offset<flatbuffers::String>> consumerIds;

		for (const auto& kv : this->mapConsumers)
		{
			const auto& consumerId = kv.first;

			consumerIds.emplace_back(builder.CreateString(consumerId));
		}

		// Add mapSsrcConsumerId.
		std::vector<flatbuffers::Offset<FBS::Common::Uint32String>> mapSsrcConsumerId;

		for (const auto& kv : this->mapSsrcConsumer)
		{
			auto ssrc      = kv.first;
			auto* consumer = kv.second;

			mapSsrcConsumerId.emplace_back(
			  FBS::Common::CreateUint32StringDirect(builder, ssrc, consumer->id.c_str()));
		}

		// Add mapRtxSsrcConsumerId.
		std::vector<flatbuffers::Offset<FBS::Common::Uint32String>> mapRtxSsrcConsumerId;

		for (const auto& kv : this->mapRtxSsrcConsumer)
		{
			auto ssrc      = kv.first;
			auto* consumer = kv.second;

			mapRtxSsrcConsumerId.emplace_back(
			  FBS::Common::CreateUint32StringDirect(builder, ssrc, consumer->id.c_str()));
		}

		// Add dataProducerIds.
		std::vector<flatbuffers::Offset<flatbuffers::String>> dataProducerIds;

		for (const auto& kv : this->mapDataProducers)
		{
			const auto& dataProducerId = kv.first;

			dataProducerIds.emplace_back(builder.CreateString(dataProducerId));
		}

		// Add dataConsumerIds.
		std::vector<flatbuffers::Offset<flatbuffers::String>> dataConsumerIds;

		for (const auto& kv : this->mapDataConsumers)
		{
			const auto& dataConsumerId = kv.first;

			dataConsumerIds.emplace_back(builder.CreateString(dataConsumerId));
		}

		// Add headerExtensionIds.
		auto recvRtpHeaderExtensions = FBS::Transport::CreateRecvRtpHeaderExtensions(
		  builder,
		  this->recvRtpHeaderExtensionIds.mid != 0
		    ? flatbuffers::Optional<uint8_t>(this->recvRtpHeaderExtensionIds.mid)
		    : flatbuffers::nullopt,
		  this->recvRtpHeaderExtensionIds.rid != 0
		    ? flatbuffers::Optional<uint8_t>(this->recvRtpHeaderExtensionIds.rid)
		    : flatbuffers::nullopt,
		  this->recvRtpHeaderExtensionIds.rrid != 0
		    ? flatbuffers::Optional<uint8_t>(this->recvRtpHeaderExtensionIds.rrid)
		    : flatbuffers::nullopt,
		  this->recvRtpHeaderExtensionIds.absSendTime != 0
		    ? flatbuffers::Optional<uint8_t>(this->recvRtpHeaderExtensionIds.absSendTime)
		    : flatbuffers::nullopt,
		  this->recvRtpHeaderExtensionIds.transportWideCc01 != 0
		    ? flatbuffers::Optional<uint8_t>(this->recvRtpHeaderExtensionIds.transportWideCc01)
		    : flatbuffers::nullopt);

		auto rtpListenerOffset = this->rtpListener.FillBuffer(builder);

		flatbuffers::Offset<FBS::SctpParameters::SctpParameters> sctpParameters;
		FBS::SctpAssociation::SctpState sctpState{ FBS::SctpAssociation::SctpState::NEW };
		flatbuffers::Offset<FBS::SctpAssociation::SctpNegotiatedCapabilities> sctpNegotiatedCapabilities;

		// Add sctpListener.
		flatbuffers::Offset<FBS::Transport::SctpListener> sctpListener;

		if (this->sctpAssociation)
		{
			// Add sctpParameters.
			sctpParameters = this->sctpAssociation->FillBuffer(builder);

			// NOTE: There is never permanent FAILED state.
			switch (this->sctpAssociation->GetAssociationState())
			{
				case RTC::SCTP::Types::AssociationState::NEW:
				{
					sctpState = FBS::SctpAssociation::SctpState::NEW;

					break;
				}

				case RTC::SCTP::Types::AssociationState::CONNECTING:
				{
					sctpState = FBS::SctpAssociation::SctpState::CONNECTING;

					break;
				}

				case RTC::SCTP::Types::AssociationState::CONNECTED:
				{
					sctpState = FBS::SctpAssociation::SctpState::CONNECTED;

					break;
				}

				case RTC::SCTP::Types::AssociationState::SHUTTING_DOWN:
				case RTC::SCTP::Types::AssociationState::CLOSED:
				{
					sctpState = FBS::SctpAssociation::SctpState::CLOSED;

					break;
				}
			}

			// Add sctpNegotiatedCapabilities.
			sctpNegotiatedCapabilities = FBS::SctpAssociation::CreateSctpNegotiatedCapabilities(
			  builder,
			  this->sctpAssociation->GetNegotiatedMaxOutboundStreams(),
			  this->sctpAssociation->GetNegotiatedMaxInboundStreams());

			// Add sctpListener.
			sctpListener = this->sctpListener.FillBuffer(builder);
		}

		// Add traceEventTypes.
		std::vector<FBS::Transport::TraceEventType> traceEventTypes;

		if (this->traceEventTypes.probation)
		{
			traceEventTypes.emplace_back(FBS::Transport::TraceEventType::PROBATION);
		}
		if (this->traceEventTypes.bwe)
		{
			traceEventTypes.emplace_back(FBS::Transport::TraceEventType::BWE);
		}

		return FBS::Transport::CreateDumpDirect(
		  builder,
		  this->id.c_str(),
		  this->direct,
		  std::addressof(producerIds),
		  std::addressof(consumerIds),
		  std::addressof(mapSsrcConsumerId),
		  std::addressof(mapRtxSsrcConsumerId),
		  std::addressof(dataProducerIds),
		  std::addressof(dataConsumerIds),
		  recvRtpHeaderExtensions,
		  rtpListenerOffset,
		  this->maxSendMessageSize,
		  this->maxReceiveMessageSize,
		  sctpParameters,
		  this->sctpAssociation ? flatbuffers::Optional<FBS::SctpAssociation::SctpState>(sctpState)
			                      : flatbuffers::nullopt,
		  sctpNegotiatedCapabilities,
		  sctpListener,
		  std::addressof(traceEventTypes));
	}

	flatbuffers::Offset<FBS::Transport::Stats> Transport::FillBufferStats(
	  flatbuffers::FlatBufferBuilder& builder)
	{
		MS_TRACE();

		const int64_t nowMs = this->shared->GetTimeMs();

		// Add sctpState.
		FBS::SctpAssociation::SctpState sctpState{ FBS::SctpAssociation::SctpState::NEW };

		// Add sctpState.
		if (this->sctpAssociation)
		{
			// NOTE: There is never permanent FAILED state.
			switch (this->sctpAssociation->GetAssociationState())
			{
				case RTC::SCTP::Types::AssociationState::NEW:
				{
					sctpState = FBS::SctpAssociation::SctpState::NEW;
					break;
				}

				case RTC::SCTP::Types::AssociationState::CONNECTING:
				{
					sctpState = FBS::SctpAssociation::SctpState::CONNECTING;
					break;
				}

				case RTC::SCTP::Types::AssociationState::CONNECTED:
				{
					sctpState = FBS::SctpAssociation::SctpState::CONNECTED;
					break;
				}

				case RTC::SCTP::Types::AssociationState::SHUTTING_DOWN:
				case RTC::SCTP::Types::AssociationState::CLOSED:
				{
					sctpState = FBS::SctpAssociation::SctpState::CLOSED;
					break;
				}
			}
		}

		flatbuffers::Optional<uint64_t> availableOutgoingBitrate{ flatbuffers::nullopt };

		if (this->senderTransportCongestionController)
		{
			availableOutgoingBitrate =
			  static_cast<uint64_t>(this->senderTransportCongestionController->GetAvailableBitrate());
		}

		flatbuffers::Optional<uint64_t> availableIncomingBitrate{ flatbuffers::nullopt };

		if (this->receiverTransportCongestionController)
		{
			const auto bitrate = this->receiverTransportCongestionController->GetAvailableBitrate();

			if (bitrate.has_value())
			{
				availableIncomingBitrate = static_cast<uint64_t>(bitrate.value());
			}
		}

		// The bitrate bounds only mean something while there is something applying
		// them, so they are told apart by whether it is there rather than by their
		// value.
		const bool outgoingBitratesApply = this->senderTransportCongestionController != nullptr;
		const bool incomingBitratesApply = this->receiverTransportCongestionController != nullptr;

		const std::optional<int64_t> rttMs = this->rttCalculator.GetLastRttMs();

		return FBS::Transport::CreateStatsDirect(
		  builder,
		  // transportId.
		  this->id.c_str(),
		  // timestamp.
		  static_cast<uint64_t>(nowMs),
		  // sctpState.
		  this->sctpAssociation ? flatbuffers::Optional<FBS::SctpAssociation::SctpState>(sctpState)
			                      : flatbuffers::nullopt,
		  // bytesReceived.
		  this->recvTransmission.GetBytes(),
		  // recvBitrate.
		  static_cast<uint64_t>(this->recvTransmission.GetRate(nowMs).value_or(0)),
		  // bytesSent.
		  this->sendTransmission.GetBytes(),
		  // sendBitrate.
		  static_cast<uint64_t>(this->sendTransmission.GetRate(nowMs).value_or(0)),
		  // rtpBytesReceived.
		  this->recvRtpTransmission.GetBytes(),
		  // rtpRecvBitrate.
		  static_cast<uint64_t>(this->recvRtpTransmission.GetBitrate(nowMs).value_or(0)),
		  // rtpBytesSent.
		  this->sendRtpTransmission.GetBytes(),
		  // rtpSendBitrate.
		  static_cast<uint64_t>(this->sendRtpTransmission.GetBitrate(nowMs).value_or(0)),
		  // rtxBytesReceived.
		  this->recvRtxTransmission.GetBytes(),
		  // rtxRecvBitrate.
		  static_cast<uint64_t>(this->recvRtxTransmission.GetBitrate(nowMs).value_or(0)),
		  // rtxBytesSent.
		  this->sendRtxTransmission.GetBytes(),
		  // rtxSendBitrate.
		  static_cast<uint64_t>(this->sendRtxTransmission.GetBitrate(nowMs).value_or(0)),
		  // probationBytesSent.
		  this->sendProbationTransmission.GetBytes(),
		  // probationSendBitrate.
		  static_cast<uint64_t>(this->sendProbationTransmission.GetBitrate(nowMs).value_or(0)),
		  // availableOutgoingBitrate.
		  availableOutgoingBitrate,
		  // availableIncomingBitrate.
		  availableIncomingBitrate,
		  // maxIncomingBitrate.
		  incomingBitratesApply
		    ? flatbuffers::Optional<uint64_t>(static_cast<uint64_t>(this->maxIncomingBitrate))
				: flatbuffers::nullopt,
		  // maxOutgoingBitrate.
		  outgoingBitratesApply
		    ? flatbuffers::Optional<uint64_t>(static_cast<uint64_t>(this->maxOutgoingBitrate))
				: flatbuffers::nullopt,
		  // minOutgoingBitrate.
		  outgoingBitratesApply
		    ? flatbuffers::Optional<uint64_t>(static_cast<uint64_t>(this->minOutgoingBitrate))
				: flatbuffers::nullopt,
		  // roundTripTime.
		  rttMs.has_value() ? flatbuffers::Optional<float>(static_cast<float>(rttMs.value()))
			                  : flatbuffers::nullopt);
	}

	void Transport::HandleRequest(Channel::ChannelRequest* request)
	{
		MS_TRACE();

		switch (request->method)
		{
			case Channel::ChannelRequest::Method::TRANSPORT_SET_MAX_INCOMING_BITRATE:
			{
				const auto* body = request->data->body_as<FBS::Transport::SetMaxIncomingBitrateRequest>();

				// NOTE: The API gives an unsigned 64 bits bitrate, so it is clamped here.
				const auto maxIncomingBitrate =
				  static_cast<int64_t>(std::min<uint64_t>(body->maxIncomingBitrate(), AbsoluteMaxBitrate));

				// NOTE: A zero lifts the cap rather than setting one.
				this->maxIncomingBitrate = maxIncomingBitrate != 0 ? maxIncomingBitrate : AbsoluteMaxBitrate;

				MayFixBweBitrateLimits();

				MS_DEBUG_TAG(bwe, "maximum incoming bitrate set to %" PRIi64, this->maxIncomingBitrate);

				request->Accept();

				if (this->receiverTransportCongestionController)
				{
					// The cap travels to the remote peer in a REMB, so a peer that did not
					// negotiate REMB cannot be told about it by any means.
					//
					// NOTE: There is nothing to lift if it was never applied.
					if (this->recvSupportsRemb)
					{
						this->receiverTransportCongestionController->SetMaxIncomingBitrate(
						  this->maxIncomingBitrate != AbsoluteMaxBitrate
						    ? std::optional<int64_t>(this->maxIncomingBitrate)
								: std::nullopt);
					}
					else if (this->maxIncomingBitrate != AbsoluteMaxBitrate)
					{
						MS_WARN_TAG(
						  bwe, "cannot apply maximum incoming bitrate since remote peer does not support REMB");
					}
				}

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_SET_MAX_OUTGOING_BITRATE:
			{
				const auto* body = request->data->body_as<FBS::Transport::SetMaxOutgoingBitrateRequest>();

				// NOTE: The API gives an unsigned 64 bits bitrate, so it is clamped here.
				const auto bitrate =
				  static_cast<int64_t>(std::min<uint64_t>(body->maxOutgoingBitrate(), AbsoluteMaxBitrate));

				// NOTE: A zero lifts the cap rather than setting one.
				this->maxOutgoingBitrate = bitrate != 0 ? bitrate : AbsoluteMaxBitrate;

				MayFixBweBitrateLimits();

				MS_DEBUG_TAG(bwe, "maximum outgoing bitrate set to %" PRIi64, this->maxOutgoingBitrate);

				if (this->senderTransportCongestionController)
				{
					this->senderTransportCongestionController->SetBitrateLimits(
					  this->minOutgoingBitrate, this->maxOutgoingBitrate);
				}

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_SET_MIN_OUTGOING_BITRATE:
			{
				const auto* body = request->data->body_as<FBS::Transport::SetMinOutgoingBitrateRequest>();

				// NOTE: The API gives an unsigned 64 bits bitrate, so it is clamped here.
				const auto bitrate =
				  static_cast<int64_t>(std::min<uint64_t>(body->minOutgoingBitrate(), AbsoluteMaxBitrate));

				// NOTE: A zero lifts the floor rather than setting one.
				this->minOutgoingBitrate = bitrate != 0 ? bitrate : AbsoluteMinBitrate;

				MayFixBweBitrateLimits();

				MS_DEBUG_TAG(bwe, "minimum outgoing bitrate set to %" PRIi64, this->minOutgoingBitrate);

				if (this->senderTransportCongestionController)
				{
					this->senderTransportCongestionController->SetBitrateLimits(
					  this->minOutgoingBitrate, this->maxOutgoingBitrate);
				}

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_PRODUCE:
			{
				const auto* body      = request->data->body_as<FBS::Transport::ProduceRequest>();
				const auto producerId = body->producerId()->str();

				CheckNoProducer(producerId, request->methodCStr);

				// This may throw.
				auto* producer = new RTC::Producer(this->shared, producerId, this, body);

				// Insert the Producer into the RtpListener.
				// This may throw. If so, delete the Producer and throw.
				try
				{
					this->rtpListener.AddProducer(producer);
				}
				catch (const MediaSoupError& error)
				{
					delete producer;

					throw;
				}

				// Notify the listener.
				// This may throw if a Producer with same id already exists.
				try
				{
					this->listener->OnTransportNewProducer(this, producer);
				}
				catch (const MediaSoupError& error)
				{
					this->rtpListener.RemoveProducer(producer);

					delete producer;

					throw;
				}

				// Insert into the map.
				this->mapProducers[producerId] = producer;

				// Take this Producer into account for the capture time estimation of its
				// sender.
				{
					const auto& cname = producer->GetRtpParameters().rtcp.cname;

					this->mapCnameRemoteCaptureTimeEstimator[cname].UpdateSource(
					  producer->GetRtpHeaderExtensionIds().absCaptureTime != 0);
				}

				MS_DEBUG_DEV("Producer created [producerId:%s]", producerId.c_str());

				// Take the transport related RTP header extensions of the Producer and
				// add them to the Transport.
				// NOTE: Producer::GetRtpHeaderExtensionIds() returns the original
				// header extension ids of the Producer (and not their mapped values).
				const auto& producerRtpHeaderExtensionIds = producer->GetRtpHeaderExtensionIds();

				if (producerRtpHeaderExtensionIds.mid != 0)
				{
					this->recvRtpHeaderExtensionIds.mid = producerRtpHeaderExtensionIds.mid;
				}

				if (producerRtpHeaderExtensionIds.rid != 0)
				{
					this->recvRtpHeaderExtensionIds.rid = producerRtpHeaderExtensionIds.rid;
				}

				if (producerRtpHeaderExtensionIds.rrid != 0)
				{
					this->recvRtpHeaderExtensionIds.rrid = producerRtpHeaderExtensionIds.rrid;
				}

				if (producerRtpHeaderExtensionIds.absSendTime != 0)
				{
					this->recvRtpHeaderExtensionIds.absSendTime = producerRtpHeaderExtensionIds.absSendTime;
				}

				if (producerRtpHeaderExtensionIds.transportWideCc01 != 0)
				{
					this->recvRtpHeaderExtensionIds.transportWideCc01 =
					  producerRtpHeaderExtensionIds.transportWideCc01;
				}

				if (producerRtpHeaderExtensionIds.dependencyDescriptor != 0)
				{
					this->recvRtpHeaderExtensionIds.dependencyDescriptor =
					  producerRtpHeaderExtensionIds.dependencyDescriptor;
				}

				// NOTE: Not transport related, but needed here so that received packets carry
				// its id and `RtpStreamRecv` can read the extension off them.
				if (producerRtpHeaderExtensionIds.absCaptureTime != 0)
				{
					this->recvRtpHeaderExtensionIds.absCaptureTime =
					  producerRtpHeaderExtensionIds.absCaptureTime;
				}

				// Create status response.
				auto responseOffset = FBS::Transport::CreateProduceResponse(
				  request->GetBufferBuilder(), FBS::RtpParameters::Type(producer->GetType()));

				request->Accept(FBS::Response::Body::Transport_ProduceResponse, responseOffset);

				if (!this->recvSupportsTransportCc)
				{
					this->recvSupportsTransportCc = producer->SupportsTransportCc();
				}

				if (!this->recvSupportsRemb)
				{
					this->recvSupportsRemb = producer->SupportsRemb();
				}

				// NOTE: A Transport that gets its first Producer while not yet connected
				// builds it in Transport::Connected() instead.
				const bool receiverControllerCreated = MayCreateReceiverTransportCongestionController();

				// The cap travels to the remote peer in a REMB, so it only becomes
				// applicable once a Producer brings REMB, which may well be this one.
				if (this->receiverTransportCongestionController && this->maxIncomingBitrate != AbsoluteMaxBitrate)
				{
					if (this->recvSupportsRemb)
					{
						this->receiverTransportCongestionController->SetMaxIncomingBitrate(
						  this->maxIncomingBitrate);
					}
					// NOTE: Only on the Producer that builds it, since every later one
					// would say the same thing again.
					else if (receiverControllerCreated)
					{
						MS_WARN_TAG(
						  bwe, "cannot apply maximum incoming bitrate since remote peer does not support REMB");
					}
				}

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_CONSUME:
			{
				const auto* body      = request->data->body_as<FBS::Transport::ConsumeRequest>();
				const auto producerId = body->producerId()->str();
				const auto consumerId = body->consumerId()->str();

				CheckNoConsumer(consumerId, request->methodCStr);

				// This may throw.
				auto* consumer = new RTC::Consumer(this->shared, consumerId, producerId, this, body);

				// Notify the listener.
				// This may throw if no Producer is found.
				try
				{
					this->listener->OnTransportNewConsumer(this, consumer, producerId);
				}
				catch (const MediaSoupError& error)
				{
					delete consumer;

					throw;
				}

				// Insert into the maps.
				this->mapConsumers[consumerId] = consumer;

				for (auto ssrc : consumer->GetMediaSsrcs())
				{
					this->mapSsrcConsumer[ssrc] = consumer;
				}

				for (auto ssrc : consumer->GetRtxSsrcs())
				{
					this->mapRtxSsrcConsumer[ssrc] = consumer;
				}

				MS_DEBUG_DEV(
				  "Consumer created [consumerId:%s, producerId:%s]", consumerId.c_str(), producerId.c_str());

				flatbuffers::Offset<FBS::Consumer::ConsumerLayers> preferredLayersOffset;

				const auto preferredLayers = consumer->GetPreferredLayers();

				if (preferredLayers.spatial > -1 && preferredLayers.temporal > -1)
				{
					preferredLayersOffset = FBS::Consumer::CreateConsumerLayers(
					  request->GetBufferBuilder(),
					  static_cast<uint8_t>(preferredLayers.spatial),
					  static_cast<uint8_t>(preferredLayers.temporal));
				}

				auto scoreOffset    = consumer->FillBufferScore(request->GetBufferBuilder());
				auto responseOffset = FBS::Transport::CreateConsumeResponse(
				  request->GetBufferBuilder(),
				  consumer->IsPaused(),
				  consumer->IsProducerPaused(),
				  scoreOffset,
				  preferredLayersOffset);

				request->Accept(FBS::Response::Body::Transport_ConsumeResponse, responseOffset);

				if (!this->sendSupportsTransportCc)
				{
					this->sendSupportsTransportCc = consumer->SupportsTransportCc();
				}

				if (!this->sendSupportsRemb)
				{
					this->sendSupportsRemb = consumer->SupportsRemb();
				}

				// NOTE: A Transport that gets its first Consumer while not yet connected
				// builds it in Transport::Connected() instead.
				MayCreateOrUpdateSenderTransportCongestionController();

				// NOTE: When this very call is the one that built it, the loop inside it
				// has already told this Consumer, since it is in the map by now.
				if (this->senderTransportCongestionController)
				{
					consumer->SetBweManaged(true);
				}

				if (IsConnected())
				{
					consumer->TransportConnected();
				}

				// A Consumer that is already putting something out changes what there is
				// to give out. One created paused offers nothing yet, and asks on its own
				// when it is resumed.
				if (this->senderTransportCongestionController && consumer->IsActive())
				{
					DistributeAvailableOutgoingBitrate();
					ComputeOutgoingDesiredBitrate();
				}

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_PRODUCE_DATA:
			{
				// Early check. The Transport must support SCTP or be direct.
				if (!this->sctpAssociation && !this->direct)
				{
					MS_THROW_ERROR("SCTP not enabled and not a direct Transport");
				}

				const auto* body          = request->data->body_as<FBS::Transport::ProduceDataRequest>();
				const auto dataProducerId = body->dataProducerId()->str();

				// This may throw.
				CheckNoDataProducer(dataProducerId, request->methodCStr);

				// This may throw.
				auto* dataProducer = new RTC::DataProducer(
				  this->shared, dataProducerId, this->maxReceiveMessageSize, this, body);

				// Verify the type of the DataProducer.
				switch (dataProducer->GetType())
				{
					case RTC::DataProducer::Type::SCTP:
					{
						if (!this->sctpAssociation)
						{
							delete dataProducer;

							MS_THROW_TYPE_ERROR(
							  "cannot create a DataProducer of type 'sctp', SCTP not enabled in this Transport");
						}

						break;
					}

					case RTC::DataProducer::Type::DIRECT:
					{
						if (!this->direct)
						{
							delete dataProducer;

							MS_THROW_TYPE_ERROR(
							  "cannot create a DataProducer of type 'direct', not a direct Transport");
						}

						break;
					}
				}

				if (dataProducer->GetType() == RTC::DataProducer::Type::SCTP)
				{
					// Insert the DataProducer into the SctpListener.
					// This may throw. If so, delete the DataProducer and throw.
					try
					{
						this->sctpListener.AddDataProducer(dataProducer);
					}
					catch (const MediaSoupError& error)
					{
						delete dataProducer;

						throw;
					}
				}

				// Notify the listener.
				// This may throw if a DataProducer with same id already exists.
				try
				{
					this->listener->OnTransportNewDataProducer(this, dataProducer);
				}
				catch (const MediaSoupError& error)
				{
					if (dataProducer->GetType() == RTC::DataProducer::Type::SCTP)
					{
						this->sctpListener.RemoveDataProducer(dataProducer);
					}

					delete dataProducer;

					throw;
				}

				// Insert into the map.
				this->mapDataProducers[dataProducerId] = dataProducer;

				MS_DEBUG_DEV("DataProducer created [dataProducerId:%s]", dataProducerId.c_str());

				auto dumpOffset = dataProducer->FillBuffer(request->GetBufferBuilder());

				request->Accept(FBS::Response::Body::DataProducer_DumpResponse, dumpOffset);

				if (dataProducer->GetType() == RTC::DataProducer::Type::SCTP)
				{
					// Tell to the SCTP association.
					this->sctpAssociation->MayConnect();
				}

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_CONSUME_DATA:
			{
				// Early check. The Transport must support SCTP or be direct.
				if (!this->sctpAssociation && !this->direct)
				{
					MS_THROW_ERROR("SCTP not enabled and not a direct Transport");
				}

				const auto* body          = request->data->body_as<FBS::Transport::ConsumeDataRequest>();
				const auto dataProducerId = body->dataProducerId()->str();
				const auto dataConsumerId = body->dataConsumerId()->str();

				// This may throw.
				CheckNoDataConsumer(dataConsumerId, request->methodCStr);

				// This may throw.
				auto* dataConsumer = new RTC::DataConsumer(
				  this->shared,
				  dataConsumerId,
				  dataProducerId,
				  this,
				  body,
				  this->maxSendMessageSize,
				  this->IsPipe());

				// Verify the type of the DataConsumer.
				switch (dataConsumer->GetType())
				{
					case RTC::DataConsumer::Type::SCTP:
					{
						if (!this->sctpAssociation)
						{
							delete dataConsumer;

							MS_THROW_TYPE_ERROR(
							  "cannot create a DataConsumer of type 'sctp', SCTP not enabled in this Transport");
						}

						try
						{
							// This may throw.
							CheckNoSctpDataConsumer(
							  dataConsumer->GetSctpStreamParameters().streamId, request->methodCStr);
						}
						catch (const MediaSoupError& error)
						{
							delete dataConsumer;

							throw;
						}

						break;
					}

					case RTC::DataConsumer::Type::DIRECT:
					{
						if (!this->direct)
						{
							delete dataConsumer;

							MS_THROW_TYPE_ERROR(
							  "cannot create a DataConsumer of type 'direct', not a direct Transport");
						}

						break;
					}
				}

				// Notify the listener.
				// This may throw if no DataProducer is found.
				try
				{
					this->listener->OnTransportNewDataConsumer(this, dataConsumer, dataProducerId);
				}
				catch (const MediaSoupError& error)
				{
					delete dataConsumer;

					throw;
				}

				// Insert into the maps.
				this->mapDataConsumers[dataConsumerId] = dataConsumer;

				if (dataConsumer->GetType() == RTC::DataConsumer::Type::SCTP)
				{
					this->mapSctpStreamIdDataConsumers[dataConsumer->GetSctpStreamParameters().streamId] =
					  dataConsumer;
				}

				MS_DEBUG_DEV(
				  "DataConsumer created [dataConsumerId:%s, dataProducerId:%s]",
				  dataConsumerId.c_str(),
				  dataProducerId.c_str());

				auto dumpOffset = dataConsumer->FillBuffer(request->GetBufferBuilder());

				request->Accept(FBS::Response::Body::DataConsumer_DumpResponse, dumpOffset);

				if (dataConsumer->GetType() == RTC::DataConsumer::Type::SCTP)
				{
					// Tell to the SCTP association.
					this->sctpAssociation->MayConnect();
				}

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_ENABLE_TRACE_EVENT:
			{
				const auto* body = request->data->body_as<FBS::Transport::EnableTraceEventRequest>();

				// Reset traceEventTypes.
				TraceEventTypes newTraceEventTypes;

				for (const auto& type : *body->events())
				{
					switch (type)
					{
						case FBS::Transport::TraceEventType::PROBATION:
						{
							newTraceEventTypes.probation = true;

							break;
						}

						case FBS::Transport::TraceEventType::BWE:
						{
							newTraceEventTypes.bwe = true;

							break;
						}
					}
				}

				this->traceEventTypes = newTraceEventTypes;

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_CLOSE_PRODUCER:
			{
				const auto* body = request->data->body_as<FBS::Transport::CloseProducerRequest>();

				// This may throw.
				RTC::Producer* producer =
				  AssertAndGetProducerById(body->producerId()->str(), request->methodCStr);

				// Remove it from the RtpListener.
				this->rtpListener.RemoveProducer(producer);

				// Remove it from the map.
				this->mapProducers.erase(producer->id);

				// Remove the capture time estimator of its sender once its last Producer
				// is gone.
				{
					const auto& cname = producer->GetRtpParameters().rtcp.cname;

					const bool cnameStillInUse = std::ranges::any_of(
					  this->mapProducers,
					  [&cname](const auto& kv)
					  {
						  const auto* otherProducer = kv.second;

						  return otherProducer->GetRtpParameters().rtcp.cname == cname;
					  });

					if (!cnameStillInUse)
					{
						this->mapCnameRemoteCaptureTimeEstimator.erase(cname);
					}
				}

				// Tell the child class to clear associated SSRCs.
				for (const auto& kv : producer->GetRtpStreams())
				{
					auto* rtpStream = kv.first;

					RecvStreamClosed(rtpStream->GetSsrc());

					if (this->receiverTransportCongestionController)
					{
						this->receiverTransportCongestionController->RemoveStream(rtpStream->GetSsrc());
					}

					if (rtpStream->HasRtx())
					{
						RecvStreamClosed(rtpStream->GetRtxSsrc());

						if (this->receiverTransportCongestionController)
						{
							this->receiverTransportCongestionController->RemoveStream(rtpStream->GetRtxSsrc());
						}
					}
				}

				// Notify the listener.
				this->listener->OnTransportProducerClosed(this, producer);

				MS_DEBUG_DEV("Producer closed [producerId:%s]", producer->id.c_str());

				// Delete it.
				delete producer;

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_CLOSE_CONSUMER:
			{
				const auto* body = request->data->body_as<FBS::Transport::CloseConsumerRequest>();

				// This may throw.
				RTC::Consumer* consumer =
				  AssertAndGetConsumerById(body->consumerId()->str(), request->methodCStr);

				// Remove it from the maps.
				this->mapConsumers.erase(consumer->id);

				for (auto ssrc : consumer->GetMediaSsrcs())
				{
					this->mapSsrcConsumer.erase(ssrc);

					// Tell the child class to clear associated SSRCs.
					SendStreamClosed(ssrc);
				}

				for (auto ssrc : consumer->GetRtxSsrcs())
				{
					this->mapRtxSsrcConsumer.erase(ssrc);

					// Tell the child class to clear associated SSRCs.
					SendStreamClosed(ssrc);
				}

				// Notify the listener.
				this->listener->OnTransportConsumerClosed(this, consumer);

				MS_DEBUG_DEV("Consumer closed [consumerId:%s]", consumer->id.c_str());

				// Delete it.
				delete consumer;

				request->Accept();

				// What this one was putting in the link goes to whoever can use it, and
				// what the remaining Consumers want is now less, which is an input of the
				// probing.
				if (this->senderTransportCongestionController)
				{
					DistributeAvailableOutgoingBitrate();
					ComputeOutgoingDesiredBitrate();
				}

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_CLOSE_DATAPRODUCER:
			{
				if (!this->sctpAssociation && !this->direct)
				{
					MS_THROW_ERROR("cannot close DataProducer, SCTP not enabled and not a direct Transport");
				}

				const auto* body = request->data->body_as<FBS::Transport::CloseDataProducerRequest>();

				// This may throw.
				RTC::DataProducer* dataProducer =
				  AssertAndGetDataProducerById(body->dataProducerId()->str(), request->methodCStr);

				if (dataProducer->GetType() == RTC::DataProducer::Type::SCTP)
				{
					// Remove it from the SctpListener.
					this->sctpListener.RemoveDataProducer(dataProducer);
				}

				// Remove it from the map.
				this->mapDataProducers.erase(dataProducer->id);

				// https://datatracker.ietf.org/doc/html/rfc8831#section-6.7
				//
				// "Closing of a data channel MUST be signaled by resetting the corresponding
				// outgoing streams [RFC6525]. This means that if one side decides to close
				// the data channel, it resets the corresponding outgoing stream. When the
				// peer sees that an incoming stream was reset, it also resets its
				// corresponding outgoing stream."
				if (this->sctpAssociation && this->sctpAssociation->IsDataChannel())
				{
					this->sctpAssociation->ResetStreams(
					  std::array<uint16_t, 1>{ dataProducer->GetSctpStreamParameters().streamId });
				}

				// Notify the listener.
				this->listener->OnTransportDataProducerClosed(this, dataProducer);

				MS_DEBUG_DEV("DataProducer closed [dataProducerId:%s]", dataProducer->id.c_str());

				// Delete it.
				delete dataProducer;

				request->Accept();

				break;
			}

			case Channel::ChannelRequest::Method::TRANSPORT_CLOSE_DATACONSUMER:
			{
				if (!this->sctpAssociation && !this->direct)
				{
					MS_THROW_ERROR("cannot close DataConsumer, SCTP not enabled and not a direct Transport");
				}

				const auto* body = request->data->body_as<FBS::Transport::CloseDataConsumerRequest>();

				// This may throw.
				RTC::DataConsumer* dataConsumer =
				  AssertAndGetDataConsumerById(body->dataConsumerId()->str(), request->methodCStr);

				// Remove it from the maps.
				this->mapDataConsumers.erase(dataConsumer->id);

				if (dataConsumer->GetType() == RTC::DataConsumer::Type::SCTP)
				{
					this->mapSctpStreamIdDataConsumers.erase(dataConsumer->GetSctpStreamParameters().streamId);
				}

				if (this->sctpAssociation)
				{
					// NOTE: This must be called after removing data consumers from the maps,
					// otherwise if `OnAssociationStreamBufferedAmountLow()` was triggered it
					// would end up emitting an event associated to an already closed data
					// consumer.
					this->sctpAssociation->ResetStreams(
					  std::array<uint16_t, 1>{ dataConsumer->GetSctpStreamParameters().streamId });
				}

				// Notify the listener.
				this->listener->OnTransportDataConsumerClosed(this, dataConsumer);

				MS_DEBUG_DEV("DataConsumer closed [dataConsumerId:%s]", dataConsumer->id.c_str());

				// Delete it.
				delete dataConsumer;

				request->Accept();

				break;
			}

			default:
			{
				MS_THROW_ERROR("unknown method '%s'", request->methodCStr);
			}
		}
	}

	void Transport::HandleNotification(Channel::ChannelNotification* notification)
	{
		MS_TRACE();

		switch (notification->data->event())
		{
			default:
			{
				MS_ERROR("unknown event '%s'", notification->eventCStr);
			}
		}
	}

	void Transport::SetDestroying()
	{
		MS_TRACE();

		if (this->sctpAssociation)
		{
			// NOTE: We don't invoke `Shutdown()` but `Close()` in the SCTP association
			// because at this point we are closing everything and we won't have any
			// chance to complete the SCTP SHUTDOWN + SHUTDOWN_ACK + SHUTDOWN_COMPLETE
			// dance, so we invoke `Close()` which just sends a SCTP ABORT.
			this->sctpAssociation->Close();
		}

		this->isDestroying = true;
	}

	void Transport::Connected()
	{
		MS_TRACE();

		// NOTE: This goes before telling the Consumers, since the round below needs
		// someone to say how much there is to give out.
		MayCreateOrUpdateSenderTransportCongestionController();
		MayCreateReceiverTransportCongestionController();

		// Tell all Consumers.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			consumer->TransportConnected();
		}

		// A Consumer offers nothing until its transport is connected, so this is the
		// first instant there is anything to give out. It goes here, once they have all
		// been told, rather than in each Consumer as it is told: that would hand the
		// whole bitrate to the first one and take most of it back on the next round,
		// with a key frame requested on each turn.
		//
		// NOTE: There may be no controller, since a Transport that connects before its
		// first Consumer gets one in HandleRequest() instead.
		if (this->senderTransportCongestionController)
		{
			DistributeAvailableOutgoingBitrate();
			ComputeOutgoingDesiredBitrate();
		}

		// Tell the SctpAssociation.
		if (this->sctpAssociation)
		{
			this->sctpAssociation->MayConnect();
		}

		// Start the RTCP timer.
		this->rtcpTimer->Start(RTC::RTCP::MaxVideoIntervalMs / 2);
	}

	void Transport::Disconnected()
	{
		MS_TRACE();

		// Tell all Consumers.
		for (auto& kv : this->mapConsumers)
		{
			auto* consumer = kv.second;

			consumer->TransportDisconnected();
		}

		// Stop the RTCP timer.
		this->rtcpTimer->Stop();

		if (this->senderTransportCongestionController)
		{
			// NOTE: Telling it that the network is gone does not stop the probing by
			// itself, since the estimator keeps deciding whether a burst is due.
			this->senderTransportCongestionController->SetEnableProbing(false);
			this->senderTransportCongestionController->SetNetworkAvailable(false);
		}
	}

	void Transport::RouteChanged()
	{
		MS_TRACE();

		// NOTE: Before being connected there is no controller yet, and the one
		// created later takes the overhead of the path in use at that moment.
		if (this->senderTransportCongestionController)
		{
			// The new path may differ in IP version or in UDP vs TCP, and so in the
			// bytes each packet carries on top of its own length.
			this->senderTransportCongestionController->SetPacketOverhead(GetPacketOverhead());
		}
	}

	void Transport::ReceiveRtpPacket(RTC::RTP::Packet* packet, int64_t receivedAtUs)
	{
		MS_TRACE();

#ifdef MS_RTC_LOGGER_RTP
		packet->logger.timeMs           = Utils::Time::TimeUsToMs(receivedAtUs);
		packet->logger.recvTransportId  = this->id;
		packet->logger.recvRtpTimestamp = packet->GetTimestamp();
		packet->logger.recvSeqNumber    = packet->GetSequenceNumber();
#endif

		// Apply the Transport RTP header extension ids so the RTP listener can use
		// them.
		packet->AssignExtensionIds(this->recvRtpHeaderExtensionIds);

		// Get the associated Producer.
		RTC::Producer* producer = this->rtpListener.GetProducer(packet);

		// NOTE: Done before giving up on a packet with no Producer, since its
		// arrival still has to be reported back. A remote peer may well close a
		// Producer over a path of its own while its packets are still in flight,
		// and a hole in the transport-cc feedback would tell it that the link lost
		// them.
		if (this->receiverTransportCongestionController)
		{
			this->receiverTransportCongestionController->ReceiveRtpPacket(
			  packet,
			  producer ? std::optional<RTC::Media::Kind>(producer->GetKind()) : std::nullopt,
			  receivedAtUs);
		}

		if (!producer)
		{
#ifdef MS_RTC_LOGGER_RTP
			packet->logger.Discarded(RTC::RtcLogger::RtpPacket::DiscardReason::PRODUCER_NOT_FOUND);
#endif

			MS_WARN_TAG(
			  rtp,
			  "no suitable Producer for received RTP packet [ssrc:%" PRIu32 ", payloadType:%" PRIu8 "]",
			  packet->GetSsrc(),
			  packet->GetPayloadType());

			// Tell the child class to remove this SSRC.
			RecvStreamClosed(packet->GetSsrc());

			delete packet;

			return;
		}

		// MS_DEBUG_DEV(
		//   "RTP packet received [ssrc:%" PRIu32 ", payloadType:%" PRIu8 ", producerId:%s]",
		//   packet->GetSsrc(),
		//   packet->GetPayloadType(),
		//   producer->id.c_str());

		// Pass the RTP packet to the corresponding Producer.
		auto result = producer->ReceiveRtpPacket(packet, receivedAtUs);

		switch (result)
		{
			case RTC::Producer::ReceiveRtpPacketResult::MEDIA:
			{
				this->recvRtpTransmission.Update(packet);

				break;
			}

			case RTC::Producer::ReceiveRtpPacketResult::RETRANSMISSION:
			{
				this->recvRtxTransmission.Update(packet);

				break;
			}

			case RTC::Producer::ReceiveRtpPacketResult::STREAM_NOT_FOUND:
			{
				// Tell the child class to remove this SSRC.
				RecvStreamClosed(packet->GetSsrc());

				break;
			}

			default:;
		}

		delete packet;
	}

	void Transport::ReceiveRtcpPacket(RTC::RTCP::Packet* packet, int64_t receivedAtUs)
	{
		MS_TRACE();

		// Handle each RTCP packet.
		while (packet)
		{
			HandleRtcpPacket(packet, receivedAtUs);

			auto* previousPacket = packet;

			packet = packet->GetNext();

			delete previousPacket;
		}
	}

	void Transport::ReceiveSctpData(const uint8_t* data, size_t len, int64_t receivedAtUs)
	{
		MS_TRACE();

		if (!this->sctpAssociation)
		{
			MS_DEBUG_TAG(sctp, "ignoring SCTP packet (SCTP not enabled)");

			return;
		}

		// Pass it to the SctpAssociation.
		this->sctpAssociation->ReceiveSctpData(data, len, receivedAtUs);
	}

	void Transport::SendSctpMessage(
	  RTC::DataConsumer* dataConsumer, RTC::SCTP::Message message, onMessageQueuedCallback cb)
	{
		MS_TRACE();

		// NOTE: The `message` must already have its `streamId` pointing to the same
		// as in the `dataConsumer` if its type is "sctp", or 0 otherwise.

		// NOTE: The thrown error is the answer here, so `cb` is deliberately not
		// invoked: whoever built it already replies to the channel request from
		// within it, and the caught error replies again, which would abort.
		if (!this->sctpAssociation)
		{
			MS_THROW_ERROR("SCTP not enabled");
		}

		const auto& sctpStreamParameters = dataConsumer->GetSctpStreamParameters();
		const RTC::SCTP::SendMessageOptions sendMessageOptions{
			.unordered = !sctpStreamParameters.ordered,
			.lifetimeMs =
			  sctpStreamParameters.ordered || !sctpStreamParameters.maxPacketLifeTime.has_value()
			    ? std::nullopt
			    : std::optional<int64_t>(sctpStreamParameters.maxPacketLifeTime.value()),
			.maxRetransmissions =
			  sctpStreamParameters.ordered ? std::nullopt : sctpStreamParameters.maxRetransmits,
			// NOTE: We don't set `lifecyleId` in production.
		};

		const auto sendStatus =
		  this->sctpAssociation->SendMessage(std::move(message), sendMessageOptions);

		switch (sendStatus)
		{
			case RTC::SCTP::Types::SendMessageStatus::SUCCESS:
			{
				if (cb)
				{
					cb(true, /*isSendBufferFull*/ false);
				}

				break;
			}

			case RTC::SCTP::Types::SendMessageStatus::ERROR_RESOURCE_EXHAUSTION:
			{
				const auto sendStatusStringView = RTC::SCTP::Types::sendMessageStatusToString(sendStatus);

				MS_WARN_2TAGS(
				  sctp,
				  message,
				  "failed to send SCTP message [sendStatus:%.*s]",
				  static_cast<int>(sendStatusStringView.size()),
				  sendStatusStringView.data());

				if (cb)
				{
					cb(false, /*isSendBufferFull*/ true);
				}

				dataConsumer->SctpSendBufferFull();

				break;
			}

			default:
			{
				const auto sendStatusStringView = RTC::SCTP::Types::sendMessageStatusToString(sendStatus);

				MS_WARN_2TAGS(
				  sctp,
				  message,
				  "failed to send SCTP message [sendStatus:%.*s]",
				  static_cast<int>(sendStatusStringView.size()),
				  sendStatusStringView.data());

				if (cb)
				{
					cb(false, /*isSendBufferFull*/ false);
				}

				break;
			}
		}
	}

	RTC::Producer* Transport::AssertAndGetProducerById(
	  const std::string& producerId, const std::string& method) const
	{
		MS_TRACE();

		const auto it = this->mapProducers.find(producerId);

		if (it == this->mapProducers.end())
		{
			MS_THROW_NOT_FOUND_ERROR("Producer not found [method:%s]", method.c_str());
		}

		return it->second;
	}

	RTC::Consumer* Transport::AssertAndGetConsumerById(
	  const std::string& consumerId, const std::string& method) const
	{
		MS_TRACE();

		const auto it = this->mapConsumers.find(consumerId);

		if (it == this->mapConsumers.end())
		{
			MS_THROW_NOT_FOUND_ERROR("Consumer not found [method:%s]", method.c_str());
		}

		return it->second;
	}

	RTC::Consumer* Transport::GetConsumerByMediaSsrc(uint32_t ssrc) const
	{
		MS_TRACE();

		const auto mapSsrcConsumerIt = this->mapSsrcConsumer.find(ssrc);

		if (mapSsrcConsumerIt == this->mapSsrcConsumer.end())
		{
			return nullptr;
		}

		auto* consumer = mapSsrcConsumerIt->second;

		return consumer;
	}

	RTC::Consumer* Transport::GetConsumerByRtxSsrc(uint32_t ssrc) const
	{
		MS_TRACE();

		const auto mapRtxSsrcConsumerIt = this->mapRtxSsrcConsumer.find(ssrc);

		if (mapRtxSsrcConsumerIt == this->mapRtxSsrcConsumer.end())
		{
			return nullptr;
		}

		auto* consumer = mapRtxSsrcConsumerIt->second;

		return consumer;
	}

	RTC::DataProducer* Transport::AssertAndGetDataProducerById(
	  const std::string& dataProducerId, const std::string& method) const
	{
		MS_TRACE();

		const auto it = this->mapDataProducers.find(dataProducerId);

		if (it == this->mapDataProducers.end())
		{
			MS_THROW_NOT_FOUND_ERROR("DataProducer not found [method:%s]", method.c_str());
		}

		return it->second;
	}

	RTC::DataConsumer* Transport::AssertAndGetDataConsumerById(
	  const std::string& dataConsumerId, const std::string& method) const
	{
		MS_TRACE();

		const auto it = this->mapDataConsumers.find(dataConsumerId);

		if (it == this->mapDataConsumers.end())
		{
			MS_THROW_NOT_FOUND_ERROR("DataConsumer not found [method:%s]", method.c_str());
		}

		return it->second;
	}

	RTC::DataConsumer* Transport::GetSctpDataConsumerByStreamId(uint16_t streamId) const
	{
		MS_TRACE();

		const auto it = this->mapSctpStreamIdDataConsumers.find(streamId);

		if (it == this->mapSctpStreamIdDataConsumers.end())
		{
			return nullptr;
		}

		return it->second;
	}

	void Transport::CheckNoProducer(const std::string& producerId, const std::string& method) const
	{
		MS_TRACE();

		if (this->mapProducers.contains(producerId))
		{
			MS_THROW_ERROR("a Producer with same producerId already exists [method:%s]", method.c_str());
		}
	}

	void Transport::CheckNoConsumer(const std::string& dataConsumerId, const std::string& method) const
	{
		MS_TRACE();

		if (this->mapConsumers.contains(dataConsumerId))
		{
			MS_THROW_ERROR("a Consumer with same consumerId already exists [method:%s]", method.c_str());
		}
	}

	void Transport::CheckNoDataProducer(const std::string& dataProducerId, const std::string& method) const
	{
		MS_TRACE();

		if (this->mapDataProducers.contains(dataProducerId))
		{
			MS_THROW_ERROR(
			  "a DataProducer with same dataProducerId already exists [method:%s]", method.c_str());
		}
	}

	void Transport::CheckNoDataConsumer(const std::string& dataConsumerId, const std::string& method) const
	{
		MS_TRACE();

		if (this->mapDataConsumers.contains(dataConsumerId))
		{
			MS_THROW_ERROR(
			  "a DataConsumer with same dataConsumerId already exists [method:%s]", method.c_str());
		}
	}

	void Transport::CheckNoSctpDataConsumer(uint16_t streamId, const std::string& method) const
	{
		MS_TRACE();

		if (this->mapSctpStreamIdDataConsumers.contains(streamId))
		{
			MS_THROW_ERROR(
			  "an SCTP DataConsumer with same streamId %" PRIu16 " already exists [method:%s]",
			  streamId,
			  method.c_str());
		}
	}

	void Transport::HandleRtcpPacket(RTC::RTCP::Packet* packet, int64_t receivedAtUs)
	{
		MS_TRACE();

		switch (packet->GetType())
		{
			case RTC::RTCP::Type::RR:
			{
				auto* rr = static_cast<RTC::RTCP::ReceiverReportPacket*>(packet);

				// What all the streams this packet reports on lost between the previous
				// report and this one, which is what the loss based estimation of the
				// sending side is drawn from.
				int64_t lostPackets{ 0 };
				int64_t expectedPackets{ 0 };

				for (auto it = rr->Begin(); it != rr->End(); ++it)
				{
					auto& report   = *it;
					auto* consumer = GetConsumerByMediaSsrc(report->GetSsrc());

					if (!consumer)
					{
						// Special case for the RTP probator.
						if (report->GetSsrc() == RTC::Consts::BweProbeRtpSsrc)
						{
							continue;
						}

						// Special case for (unused) RTCP-RR from the RTX stream.
						if (GetConsumerByRtxSsrc(report->GetSsrc()) != nullptr)
						{
							continue;
						}

						MS_DEBUG_TAG(
						  rtcp,
						  "no Consumer found for received Receiver Report [ssrc:%" PRIu32 "]",
						  report->GetSsrc());

						continue;
					}

					const auto receiverReportResult = consumer->ReceiveRtcpReceiverReport(report, receivedAtUs);

					if (receiverReportResult.rttMs.has_value())
					{
						this->rttCalculator.AddRtt(
						  static_cast<int64_t>(receiverReportResult.rttMs.value()),
						  Utils::Time::TimeUsToMs(receivedAtUs));
					}

					if (receiverReportResult.loss.has_value())
					{
						const auto& lossValue = receiverReportResult.loss.value();

						lostPackets += lossValue.lostPackets;
						expectedPackets += lossValue.expectedPackets;
					}
				}

				if (this->senderTransportCongestionController)
				{
					this->senderTransportCongestionController->ReceiveRtcpReceiverReport(
					  lostPackets, expectedPackets);
				}

				break;
			}

			case RTC::RTCP::Type::PSFB:
			{
				auto* feedback = static_cast<RTC::RTCP::FeedbackPsPacket*>(packet);

				switch (feedback->GetMessageType())
				{
					case RTC::RTCP::FeedbackPs::MessageType::PLI:
					{
						auto* consumer = GetConsumerByMediaSsrc(feedback->GetMediaSsrc());

						if (feedback->GetMediaSsrc() == RTC::Consts::BweProbeRtpSsrc)
						{
							break;
						}
						else if (!consumer)
						{
							MS_DEBUG_TAG(
							  rtcp,
							  "no Consumer found for received PLI feedback packet "
							  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
							  feedback->GetSenderSsrc(),
							  feedback->GetMediaSsrc());

							break;
						}

						MS_DEBUG_TAG(
						  rtcp,
						  "PLI received, requesting key frame for Consumer "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  feedback->GetSenderSsrc(),
						  feedback->GetMediaSsrc());

						consumer->ReceiveKeyFrameRequest(
						  RTC::RTCP::FeedbackPs::MessageType::PLI, feedback->GetMediaSsrc());

						break;
					}

					case RTC::RTCP::FeedbackPs::MessageType::FIR:
					{
						// Must iterate FIR items.
						auto* fir = static_cast<RTC::RTCP::FeedbackPsFirPacket*>(packet);

						for (auto it = fir->Begin(); it != fir->End(); ++it)
						{
							auto& item     = *it;
							auto* consumer = GetConsumerByMediaSsrc(item->GetSsrc());

							if (item->GetSsrc() == RTC::Consts::BweProbeRtpSsrc)
							{
								continue;
							}
							else if (!consumer)
							{
								MS_DEBUG_TAG(
								  rtcp,
								  "no Consumer found for received FIR feedback packet "
								  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 ", item ssrc:%" PRIu32 "]",
								  feedback->GetSenderSsrc(),
								  feedback->GetMediaSsrc(),
								  item->GetSsrc());

								continue;
							}

							MS_DEBUG_TAG(
							  rtcp,
							  "FIR received, requesting key frame for Consumer "
							  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 ", item ssrc:%" PRIu32 "]",
							  feedback->GetSenderSsrc(),
							  feedback->GetMediaSsrc(),
							  item->GetSsrc());

							consumer->ReceiveKeyFrameRequest(feedback->GetMessageType(), item->GetSsrc());
						}

						break;
					}

					case RTC::RTCP::FeedbackPs::MessageType::AFB:
					{
						auto* afb = static_cast<RTC::RTCP::FeedbackPsAfbPacket*>(feedback);

						if (afb->GetApplication() == RTC::RTCP::FeedbackPsAfbPacket::Application::REMB)
						{
							if (this->sendSupportsRemb && this->senderTransportCongestionController)
							{
								const auto* remb = static_cast<RTC::RTCP::FeedbackPsRembPacket*>(afb);

								this->senderTransportCongestionController->ReceiveEstimatedBitrate(remb->GetBitrate());
							}

							break;
						}
						else
						{
							MS_DEBUG_TAG(
							  rtcp,
							  "ignoring unsupported %s feedback PS AFB packet "
							  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
							  RTC::RTCP::FeedbackPsPacket::MessageTypeToString(feedback->GetMessageType()).c_str(),
							  feedback->GetSenderSsrc(),
							  feedback->GetMediaSsrc());

							break;
						}
					}

					default:
					{
						MS_DEBUG_TAG(
						  rtcp,
						  "ignoring unsupported %s feedback packet "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  RTC::RTCP::FeedbackPsPacket::MessageTypeToString(feedback->GetMessageType()).c_str(),
						  feedback->GetSenderSsrc(),
						  feedback->GetMediaSsrc());
					}
				}

				break;
			}

			case RTC::RTCP::Type::RTPFB:
			{
				auto* feedback = static_cast<RTC::RTCP::FeedbackRtpPacket*>(packet);
				auto* consumer = GetConsumerByMediaSsrc(feedback->GetMediaSsrc());

				// If no Consumer is found and this is not a Transport Feedback for the
				// probation SSRC or any Consumer RTX SSRC, ignore it.
				if (
				  !consumer && feedback->GetMessageType() != RTC::RTCP::FeedbackRtp::MessageType::TCC &&
				  (feedback->GetMediaSsrc() != RTC::Consts::BweProbeRtpSsrc ||
					 !GetConsumerByRtxSsrc(feedback->GetMediaSsrc())))
				{
					MS_DEBUG_TAG(
					  rtcp,
					  "no Consumer found for received feedback packet "
					  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
					  feedback->GetSenderSsrc(),
					  feedback->GetMediaSsrc());

					break;
				}

				switch (feedback->GetMessageType())
				{
					case RTC::RTCP::FeedbackRtp::MessageType::NACK:
					{
						if (!consumer)
						{
							MS_DEBUG_TAG(
							  rtcp,
							  "no Consumer found for received NACK feedback packet "
							  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
							  feedback->GetSenderSsrc(),
							  feedback->GetMediaSsrc());

							break;
						}

						auto* nackPacket = static_cast<RTC::RTCP::FeedbackRtpNackPacket*>(packet);

						consumer->ReceiveNack(nackPacket);

						break;
					}

					case RTC::RTCP::FeedbackRtp::MessageType::TCC:
					{
						auto* feedback = static_cast<RTC::RTCP::FeedbackRtpTransportPacket*>(packet);

						if (this->sendSupportsTransportCc && this->senderTransportCongestionController)
						{
							this->senderTransportCongestionController->ReceiveTransportWideCcFeedback(
							  feedback, receivedAtUs);
						}

						break;
					}

					default:
					{
						MS_DEBUG_TAG(
						  rtcp,
						  "ignoring unsupported %s feedback packet "
						  "[sender ssrc:%" PRIu32 ", media ssrc:%" PRIu32 "]",
						  RTC::RTCP::FeedbackRtpPacket::MessageTypeToString(feedback->GetMessageType()).c_str(),
						  feedback->GetSenderSsrc(),
						  feedback->GetMediaSsrc());
					}
				}

				break;
			}

			case RTC::RTCP::Type::SR:
			{
				auto* sr = static_cast<RTC::RTCP::SenderReportPacket*>(packet);

				// Even if Sender Report packet can only contains one report.
				for (auto it = sr->Begin(); it != sr->End(); ++it)
				{
					auto& report   = *it;
					auto* producer = this->rtpListener.GetProducer(report->GetSsrc());

					if (!producer)
					{
						MS_DEBUG_TAG(
						  rtcp,
						  "no Producer found for received Sender Report [ssrc:%" PRIu32 "]",
						  report->GetSsrc());

						continue;
					}

					producer->ReceiveRtcpSenderReport(report, receivedAtUs);
				}

				break;
			}

			case RTC::RTCP::Type::SDES:
			{
				// According to RFC 3550 section 6.1 "a CNAME item MUST be included in
				// each compound RTCP packet". So this is true even for compound
				// packets sent by endpoints that are not sending any RTP stream to us
				// (thus chunks in such a SDES will have an SSCR does not match with
				// any Producer created in this Transport).
				// Therefore, and given that we do nothing with SDES, just ignore them.

				break;
			}

			case RTC::RTCP::Type::BYE:
			{
				MS_DEBUG_TAG(rtcp, "ignoring received RTCP BYE");

				break;
			}

			case RTC::RTCP::Type::XR:
			{
				auto* xr = static_cast<RTC::RTCP::ExtendedReportPacket*>(packet);

				for (auto it = xr->Begin(); it != xr->End(); ++it)
				{
					auto& report = *it;

					switch (report->GetType())
					{
						case RTC::RTCP::ExtendedReportBlock::Type::DLRR:
						{
							auto* dlrr = static_cast<RTC::RTCP::DelaySinceLastRr*>(report);

							for (auto it2 = dlrr->Begin(); it2 != dlrr->End(); ++it2)
							{
								auto& ssrcInfo = *it2;

								// SSRC should be filled in the sub-block.
								if (ssrcInfo->GetSsrc() == 0)
								{
									ssrcInfo->SetSsrc(xr->GetSsrc());
								}

								auto* producer = this->rtpListener.GetProducer(ssrcInfo->GetSsrc());

								if (!producer)
								{
									MS_DEBUG_TAG(
									  rtcp,
									  "no Producer found for received Sender Extended Report [ssrc:%" PRIu32 "]",
									  ssrcInfo->GetSsrc());

									continue;
								}

								const auto rttMs = producer->ReceiveRtcpXrDelaySinceLastRr(ssrcInfo, receivedAtUs);

								if (rttMs.has_value())
								{
									this->rttCalculator.AddRtt(
									  static_cast<int64_t>(rttMs.value()), Utils::Time::TimeUsToMs(receivedAtUs));
								}
							}

							break;
						}

						case RTC::RTCP::ExtendedReportBlock::Type::RRT:
						{
							auto* rrt = static_cast<RTC::RTCP::ReceiverReferenceTime*>(report);

							for (auto& kv : this->mapConsumers)
							{
								auto* consumer = kv.second;

								consumer->ReceiveRtcpXrReceiverReferenceTime(rrt, receivedAtUs);
							}

							break;
						}

						default:;
					}
				}

				break;
			}

			default:
			{
				MS_DEBUG_TAG(
				  rtcp,
				  "unhandled RTCP type received [type:%" PRIu8 "]",
				  static_cast<uint8_t>(packet->GetType()));
			}
		}
	}

	void Transport::SendRtcp(int64_t nowUs)
	{
		MS_TRACE();

		const auto ntp = Utils::Time::TimeUsToNtp(nowUs + this->shared->GetNtpOffsetUs());

		auto packet = std::make_unique<RTC::RTCP::CompoundPacket>();

		for (auto& kv : this->mapConsumers)
		{
			auto* const consumer = kv.second;

			for (auto& senderReportItems : consumer->GetRtcp(nowUs))
			{
				// If the items do not fit in the compound packet, send it and add them
				// to a new one.
				if (!packet->AddSenderReportItems(senderReportItems))
				{
					SendRtcpCompoundPacket(packet.get());

					packet = std::make_unique<RTC::RTCP::CompoundPacket>();

					const bool added = packet->AddSenderReportItems(senderReportItems);

					MS_ASSERT(added, "Sender Report items do not fit in an empty RTCP compound packet");
				}
			}
		}

		for (auto& kv : this->mapProducers)
		{
			auto* const producer = kv.second;

			for (auto& receiverReport : producer->GetRtcp(nowUs))
			{
				// If the report does not fit in the compound packet, send it and add the
				// report to a new one.
				if (!packet->AddReceiverReport(receiverReport, ntp))
				{
					SendRtcpCompoundPacket(packet.get());

					packet = std::make_unique<RTC::RTCP::CompoundPacket>();

					const bool added = packet->AddReceiverReport(receiverReport, ntp);

					MS_ASSERT(added, "Receiver Report does not fit in an empty RTCP compound packet");
				}
			}
		}

		// Send the RTCP compound packet if there is any sender or receiver report.
		if (packet->GetReceiverReportCount() > 0 || packet->GetSenderReportCount() > 0)
		{
			SendRtcpCompoundPacket(packet.get());
		}
	}

	bool Transport::MayCreateOrUpdateSenderTransportCongestionController()
	{
		MS_TRACE();

		if (!this->enableBwe || !IsCongestionControlSupported() || !IsConnected() || this->mapConsumers.empty())
		{
			return false;
		}

		bool created{ false };

		// NOTE: Unlike the receiving side, this one does not wait for a Consumer that
		// negotiated anything in particular, since a peer that negotiated neither
		// mechanism still has its loss reported in the Receiver Reports and can still
		// be given a target out of it.
		if (!this->senderTransportCongestionController)
		{
			MS_DEBUG_TAG(bwe, "enabling SenderTransportCongestionController");

			this->senderTransportCongestionController =
			  std::make_shared<RTC::BWE::SenderTransportCongestionController>(
			    this,
			    this->shared,
			    RTC::BWE::SenderTransportCongestionController::SenderTransportCongestionControllerOptions{
			      .startBitrate = this->initialAvailableOutgoingBitrate,
			      .minBitrate   = this->minOutgoingBitrate,
			      .maxBitrate   = this->maxOutgoingBitrate });

			created = true;

			// From now on the Consumers ask for bitrate instead of helping themselves.
			for (auto& kv : this->mapConsumers)
			{
				auto* consumer = kv.second;

				consumer->SetBweManaged(true);
			}
		}

		// NOTE: The path may be another one than the last time, so the packet
		// overhead is taken again.
		this->senderTransportCongestionController->SetPacketOverhead(GetPacketOverhead());

		// Only a peer that negotiated transport-cc reports on each packet, and every
		// packet carries room for the sequence number whatever was negotiated, so
		// this is the only thing that tells both cases apart.
		this->senderTransportCongestionController->SetTransportCcFeedbackSupported(
		  this->sendSupportsTransportCc);

		// Probing only makes sense towards a peer that measures what reaches it. One
		// that negotiated transport-cc reports on each packet of the burst, which is
		// what closes the cluster the burst was sent as. One that only negotiated
		// REMB works out a bitrate of its own out of the packets arriving, and that
		// bitrate is what the burst raises: without it the peer only ever measures
		// the media being sent, so its estimate can never say that more would fit.
		// NOTE: This goes before telling it that the network is available, which is
		// what starts the RTP probing.
		this->senderTransportCongestionController->SetEnableProbing(
		  this->sendSupportsTransportCc || this->sendSupportsRemb);

		this->senderTransportCongestionController->SetNetworkAvailable(true);

		return created;
	}

	bool Transport::MayCreateReceiverTransportCongestionController()
	{
		MS_TRACE();

		if (
		  !this->enableBwe || this->receiverTransportCongestionController ||
		  !IsCongestionControlSupported() || !IsConnected() ||
		  (!this->recvSupportsTransportCc && !this->recvSupportsRemb))
		{
			return false;
		}

		MS_DEBUG_TAG(bwe, "enabling ReceiverTransportCongestionController");

		this->receiverTransportCongestionController =
		  std::make_unique<RTC::BWE::ReceiverTransportCongestionController>(
		    this,
		    this->shared,
		    RTC::BWE::ReceiverTransportCongestionController::ReceiverTransportCongestionControllerOptions{});

		// The cap travels to the remote peer in a REMB, so it could not be applied
		// before this.
		if (this->maxIncomingBitrate != AbsoluteMaxBitrate && this->recvSupportsRemb)
		{
			this->receiverTransportCongestionController->SetMaxIncomingBitrate(this->maxIncomingBitrate);
		}

		return true;
	}

	void Transport::MayFixBweBitrateLimits()
	{
		MS_TRACE();

		this->maxIncomingBitrate =
		  std::clamp(this->maxIncomingBitrate, AbsoluteMinBitrate, AbsoluteMaxBitrate);
		this->minOutgoingBitrate =
		  std::clamp(this->minOutgoingBitrate, AbsoluteMinBitrate, AbsoluteMaxBitrate);
		this->maxOutgoingBitrate =
		  std::clamp(this->maxOutgoingBitrate, AbsoluteMinBitrate, AbsoluteMaxBitrate);

		if (this->maxOutgoingBitrate < this->minOutgoingBitrate)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "raising maxOutgoingBitrate %" PRIi64 " to minOutgoingBitrate %" PRIi64,
			  this->maxOutgoingBitrate,
			  this->minOutgoingBitrate);

			this->maxOutgoingBitrate = this->minOutgoingBitrate;
		}

		const int64_t initialAvailableOutgoingBitrate = std::clamp(
		  this->initialAvailableOutgoingBitrate, this->minOutgoingBitrate, this->maxOutgoingBitrate);

		if (initialAvailableOutgoingBitrate != this->initialAvailableOutgoingBitrate)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "moving initialAvailableOutgoingBitrate %" PRIi64 " to %" PRIi64,
			  this->initialAvailableOutgoingBitrate,
			  initialAvailableOutgoingBitrate);

			this->initialAvailableOutgoingBitrate = initialAvailableOutgoingBitrate;
		}
	}

	void Transport::DistributeAvailableOutgoingBitrate()
	{
		MS_TRACE();

		MS_ASSERT(this->senderTransportCongestionController, "no SenderTransportCongestionController");

		std::vector<RTC::BWE::BitrateAllocator::Candidate*> candidates;

		candidates.reserve(this->mapConsumers.size());

		for (auto& kv : this->mapConsumers)
		{
			auto* const consumer = kv.second;

			candidates.push_back(consumer);
		}

		const int64_t availableBitrate = this->senderTransportCongestionController->GetAvailableBitrate();

		MS_DEBUG_DEV("distributing available bitrate [availableBitrate:%" PRIi64 "]", availableBitrate);

		this->bitrateAllocator.Distribute(availableBitrate, candidates);
	}

	void Transport::ComputeOutgoingDesiredBitrate()
	{
		MS_TRACE();

		MS_ASSERT(this->senderTransportCongestionController, "no SenderTransportCongestionController");

		int64_t totalDesiredBitrate{ 0 };

		for (const auto& kv : this->mapConsumers)
		{
			const auto* const consumer = kv.second;

			totalDesiredBitrate += consumer->GetDesiredBitrate();
		}

		MS_DEBUG_DEV("total desired bitrate: %" PRIi64, totalDesiredBitrate);

		this->outgoingDesiredBitrate = totalDesiredBitrate;

		this->senderTransportCongestionController->SetDesiredBitrate(totalDesiredBitrate);
	}

	void Transport::EmitTraceEventProbationType(RTC::RTP::Packet* packet) const
	{
		MS_TRACE();

		if (!this->traceEventTypes.probation)
		{
			return;
		}

		const auto rtpPacketDump =
		  packet->FillBuffer(this->shared->GetChannelNotifier()->GetBufferBuilder());

		const auto traceInfo = FBS::Transport::CreateProbationTraceInfo(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(), rtpPacketDump);

		auto notification = FBS::Transport::CreateTraceNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::Transport::TraceEventType::PROBATION,
		  static_cast<uint64_t>(this->shared->GetTimeMs()),
		  FBS::Common::TraceDirection::DIRECTION_OUT,
		  FBS::Transport::TraceInfo::ProbationTraceInfo,
		  traceInfo.Union());

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::TRANSPORT_TRACE,
		  FBS::Notification::Body::Transport_TraceNotification,
		  notification);
	}

	void Transport::EmitTraceEventBweType() const
	{
		MS_TRACE();

		if (!this->traceEventTypes.bwe)
		{
			return;
		}

		MS_ASSERT(this->senderTransportCongestionController, "no SenderTransportCongestionController");

		const auto traceInfo = FBS::Transport::CreateBweTraceInfo(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  static_cast<uint64_t>(this->outgoingDesiredBitrate),
		  static_cast<uint64_t>(this->minOutgoingBitrate),
		  static_cast<uint64_t>(this->maxOutgoingBitrate),
		  static_cast<uint64_t>(this->senderTransportCongestionController->GetAvailableBitrate()));

		auto notification = FBS::Transport::CreateTraceNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::Transport::TraceEventType::BWE,
		  static_cast<uint64_t>(this->shared->GetTimeMs()),
		  FBS::Common::TraceDirection::DIRECTION_OUT,
		  FBS::Transport::TraceInfo::BweTraceInfo,
		  traceInfo.Union());

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::TRANSPORT_TRACE,
		  FBS::Notification::Body::Transport_TraceNotification,
		  notification);
	}

	void Transport::OnProducerPaused(RTC::Producer* producer)
	{
		MS_TRACE();

		this->listener->OnTransportProducerPaused(this, producer);
	}

	void Transport::OnProducerResumed(RTC::Producer* producer)
	{
		MS_TRACE();

		this->listener->OnTransportProducerResumed(this, producer);
	}

	void Transport::OnProducerNewRtpStream(
	  RTC::Producer* producer, RTC::RTP::RtpStreamRecv* rtpStream, uint32_t mappedSsrc)
	{
		MS_TRACE();

		this->listener->OnTransportProducerNewRtpStream(this, producer, rtpStream, mappedSsrc);
	}

	void Transport::OnProducerRtpStreamScore(
	  RTC::Producer* producer, RTC::RTP::RtpStreamRecv* rtpStream, uint8_t score, uint8_t previousScore)
	{
		MS_TRACE();

		this->listener->OnTransportProducerRtpStreamScore(this, producer, rtpStream, score, previousScore);
	}

	void Transport::OnProducerRtcpSenderReport(
	  RTC::Producer* producer, RTC::RTP::RtpStreamRecv* rtpStream, bool first)
	{
		MS_TRACE();

		// Feed the capture time estimator of the sender of this Producer.
		const auto it =
		  this->mapCnameRemoteCaptureTimeEstimator.find(producer->GetRtpParameters().rtcp.cname);

		if (it != this->mapCnameRemoteCaptureTimeEstimator.end())
		{
			auto& remoteCaptureTimeEstimator = it->second;

			remoteCaptureTimeEstimator.SenderReportReceived(rtpStream);
		}

		this->listener->OnTransportProducerRtcpSenderReport(this, producer, rtpStream, first);
	}

	void Transport::OnProducerSpatialLayerActivityChanged(
	  RTC::Producer* producer, RTC::RTP::RtpStreamRecv* rtpStream, uint8_t spatialLayer, bool isActive)
	{
		MS_TRACE();

		this->listener->OnTransportProducerSpatialLayerActivityChanged(
		  this, producer, rtpStream, spatialLayer, isActive);
	}

	void Transport::OnProducerRtpPacketReceived(RTC::Producer* producer, RTC::RTP::Packet* packet)
	{
		MS_TRACE();

		this->listener->OnTransportProducerRtpPacketReceived(this, producer, packet);
	}

	void Transport::OnProducerSendRtcpPacket(RTC::Producer* /*producer*/, RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		SendRtcpPacket(packet);
	}

	uint8_t Transport::OnProducerNeedWorstRemoteFractionLost(RTC::Producer* producer, uint32_t mappedSsrc)
	{
		MS_TRACE();

		return this->listener->OnTransportNeedWorstRemoteFractionLost(this, producer, mappedSsrc);
	}

	std::optional<int64_t> Transport::OnProducerNeedLocalCaptureAtUs(
	  RTC::Producer* producer, const RTC::RTP::RtpStreamRecv* rtpStream, uint32_t ts)
	{
		MS_TRACE();

		const auto it =
		  this->mapCnameRemoteCaptureTimeEstimator.find(producer->GetRtpParameters().rtcp.cname);

		if (it == this->mapCnameRemoteCaptureTimeEstimator.end())
		{
			return std::nullopt;
		}

		auto& remoteCaptureTimeEstimator = it->second;

		return remoteCaptureTimeEstimator.GetLocalCaptureAtUs(rtpStream, ts, this->shared->GetTimeUs());
	}

	std::optional<int64_t> Transport::OnProducerNeedRemoteClockOffsetUs(const RTC::Producer* producer)
	{
		MS_TRACE();

		const auto it =
		  this->mapCnameRemoteCaptureTimeEstimator.find(producer->GetRtpParameters().rtcp.cname);

		if (it == this->mapCnameRemoteCaptureTimeEstimator.end())
		{
			return std::nullopt;
		}

		const auto& remoteCaptureTimeEstimator = it->second;
		const auto clockOffsetUs               = remoteCaptureTimeEstimator.GetClockOffsetUs();

		if (!clockOffsetUs.has_value())
		{
			return std::nullopt;
		}

		// NOTE: The estimator gives the offset against our own monotonic clock, while what
		// a receiver reconstructs out of the Sender Reports we send is the clock we
		// announce, so the distance to the NTP epoch has to be taken into account. Both
		// terms are huge and their sum is small, so they are added as microseconds before
		// anything scales them up.
		return clockOffsetUs.value() + this->shared->GetNtpOffsetUs();
	}

	void Transport::OnConsumerSendRtpPacket(RTC::Consumer* consumer, RTC::RTP::Packet* packet)
	{
		MS_TRACE();

#ifdef MS_RTC_LOGGER_RTP
		packet->logger.sendTransportId = this->id;
		packet->logger.Sent();
#endif

#ifdef MS_RTC_LOGGER_SEND_BURST
		this->sendBurstLogger.Sent(
		  this->shared->GetLoopTimeMs(),
		  packet->GetLength(),
		  /*isRetransmission*/ false,
		  /*isProbation*/ false);
#endif

		if (this->senderTransportCongestionController)
		{
			const auto sequenceNumber = this->senderTransportCongestionController->OnRtpPacketToBeSent(
			  packet, { .isAudio = consumer->GetKind() == RTC::Media::Kind::AUDIO });

			const size_t size = packet->GetLength();

			// NOTE: The socket written to may not belong to this Transport and may
			// outlive it, in which case the send callback runs once this Transport is
			// gone, so it must hold nothing of it. Same in every other send callback.
			const std::weak_ptr<RTC::BWE::SenderTransportCongestionController>
			  senderTransportCongestionControllerWeakPtr(this->senderTransportCongestionController);

			auto* shared = this->shared;

			SendRtpPacket(
			  consumer,
			  packet,
			  [senderTransportCongestionControllerWeakPtr, shared, sequenceNumber, size](bool sent) -> void
			  {
				  if (sent)
				  {
					  auto senderTransportCongestionController =
					    senderTransportCongestionControllerWeakPtr.lock();

					  if (senderTransportCongestionController)
					  {
						  senderTransportCongestionController->OnRtpPacketSent(
						    sequenceNumber, size, shared->GetTimeUs());
					  }
				  }
			  });
		}
		else
		{
			SendRtpPacket(consumer, packet);
		}

		this->sendRtpTransmission.Update(packet);
	}

	void Transport::OnConsumerRetransmitRtpPacket(RTC::Consumer* consumer, RTC::RTP::Packet* packet)
	{
		MS_TRACE();

#ifdef MS_RTC_LOGGER_SEND_BURST
		this->sendBurstLogger.Sent(
		  this->shared->GetLoopTimeMs(),
		  packet->GetLength(),
		  /*isRetransmission*/ true,
		  /*isProbation*/ false);
#endif

		if (this->senderTransportCongestionController)
		{
			// NOTE: The media SSRC is the packet one unless it goes out over RTX,
			// which is how a retransmission without RTX is told apart.
			const auto sequenceNumber = this->senderTransportCongestionController->OnRtpPacketToBeSent(
			  packet,
			  { .isAudio          = consumer->GetKind() == RTC::Media::Kind::AUDIO,
				  .isRetransmission = true,
				  .originalSsrc     = packet->GetMediaSsrc() });

			const size_t size = packet->GetLength();

			const std::weak_ptr<RTC::BWE::SenderTransportCongestionController>
			  senderTransportCongestionControllerWeakPtr(this->senderTransportCongestionController);

			auto* shared = this->shared;

			SendRtpPacket(
			  consumer,
			  packet,
			  [senderTransportCongestionControllerWeakPtr, shared, sequenceNumber, size](bool sent) -> void
			  {
				  if (sent)
				  {
					  auto senderTransportCongestionController =
					    senderTransportCongestionControllerWeakPtr.lock();

					  if (senderTransportCongestionController)
					  {
						  senderTransportCongestionController->OnRtpPacketSent(
						    sequenceNumber, size, shared->GetTimeUs());
					  }
				  }
			  });
		}
		else
		{
			SendRtpPacket(consumer, packet);
		}

		this->sendRtxTransmission.Update(packet);
	}

	void Transport::OnConsumerKeyFrameRequested(RTC::Consumer* consumer, uint32_t mappedSsrc)
	{
		MS_TRACE();

		if (!IsConnected())
		{
			MS_WARN_TAG(rtcp, "ignoring key rame request (transport not connected)");

			return;
		}

		this->listener->OnTransportConsumerKeyFrameRequested(this, consumer, mappedSsrc);
	}

	void Transport::OnConsumerNeedBitrateChange(RTC::Consumer* /*consumer*/)
	{
		MS_TRACE();

		if (this->senderTransportCongestionController)
		{
			DistributeAvailableOutgoingBitrate();
			ComputeOutgoingDesiredBitrate();
		}
	}

	void Transport::OnConsumerProducerClosed(RTC::Consumer* consumer)
	{
		MS_TRACE();

		// Remove it from the maps.
		this->mapConsumers.erase(consumer->id);

		for (auto ssrc : consumer->GetMediaSsrcs())
		{
			this->mapSsrcConsumer.erase(ssrc);

			// Tell the child class to clear associated SSRCs.
			SendStreamClosed(ssrc);
		}

		for (auto ssrc : consumer->GetRtxSsrcs())
		{
			this->mapRtxSsrcConsumer.erase(ssrc);

			// Tell the child class to clear associated SSRCs.
			SendStreamClosed(ssrc);
		}

		// Notify the listener.
		this->listener->OnTransportConsumerProducerClosed(this, consumer);

		MS_DEBUG_DEV("Consumer closed [consumerId:%s]", consumer->id.c_str());

		// Delete it.
		delete consumer;

		// What this one was putting in the link goes to whoever can use it, and what
		// the remaining Consumers want is now less, which is an input of the probing.
		if (this->senderTransportCongestionController)
		{
			DistributeAvailableOutgoingBitrate();
			ComputeOutgoingDesiredBitrate();
		}
	}

	void Transport::OnDataProducerMessageReceived(
	  RTC::DataProducer* dataProducer,
	  RTC::SCTP::Message message,
	  std::vector<uint16_t>& subchannels,
	  std::optional<uint16_t> requiredSubchannel,
	  std::optional<uint16_t> ignoredSubchannel)
	{
		MS_TRACE();

		this->listener->OnTransportDataProducerMessageReceived(
		  this, dataProducer, std::move(message), subchannels, requiredSubchannel, ignoredSubchannel);
	}

	void Transport::OnDataProducerPaused(RTC::DataProducer* dataProducer)
	{
		MS_TRACE();

		this->listener->OnTransportDataProducerPaused(this, dataProducer);
	}

	void Transport::OnDataProducerResumed(RTC::DataProducer* dataProducer)
	{
		MS_TRACE();

		this->listener->OnTransportDataProducerResumed(this, dataProducer);
	}

	void Transport::OnDataConsumerSendMessage(
	  RTC::DataConsumer* dataConsumer, RTC::SCTP::Message message, onMessageQueuedCallback cb)
	{
		MS_TRACE();

		SendMessage(dataConsumer, std::move(message), std::move(cb));
	}

	void Transport::OnDataConsumerNeedBufferedAmount(
	  const RTC::DataConsumer* dataConsumer, uint32_t& bufferedAmount) const
	{
		MS_TRACE();

		if (this->sctpAssociation)
		{
			bufferedAmount = static_cast<uint32_t>(this->sctpAssociation->GetStreamBufferedAmount(
			  dataConsumer->GetSctpStreamParameters().streamId));
		}
		else
		{
			bufferedAmount = 0;
		}
	}

	void Transport::OnDataConsumerNeedBufferedAmountLowThreshold(
	  const RTC::DataConsumer* dataConsumer, uint32_t& bufferedAmountLowThreshold) const
	{
		MS_TRACE();

		if (this->sctpAssociation)
		{
			bufferedAmountLowThreshold =
			  static_cast<uint32_t>(this->sctpAssociation->GetStreamBufferedAmountLowThreshold(
			    dataConsumer->GetSctpStreamParameters().streamId));
		}
		else
		{
			bufferedAmountLowThreshold = 0;
		}
	}

	void Transport::OnDataConsumerSetBufferedAmountLowThreshold(
	  const RTC::DataConsumer* dataConsumer, uint32_t bytes) const
	{
		MS_TRACE();

		MS_ASSERT(
		  dataConsumer->GetType() == RTC::DataConsumer::Type::SCTP, "DataConsumer must have type SCTP");

		if (this->sctpAssociation)
		{
			this->sctpAssociation->SetStreamBufferedAmountLowThreshold(
			  dataConsumer->GetSctpStreamParameters().streamId, static_cast<size_t>(bytes));
		}
	}

	void Transport::OnDataConsumerDataProducerClosed(RTC::DataConsumer* dataConsumer)
	{
		MS_TRACE();

		// Remove it from the maps.
		this->mapDataConsumers.erase(dataConsumer->id);

		if (dataConsumer->GetType() == RTC::DataConsumer::Type::SCTP)
		{
			this->mapSctpStreamIdDataConsumers.erase(dataConsumer->GetSctpStreamParameters().streamId);
		}

		// NOTE: This must be called after removing the data consumer from the maps,
		// otherwise if `OnAssociationStreamBufferedAmountLow()` was triggered it
		// would end up emitting an event associated to an already closed data
		// consumer.
		if (this->sctpAssociation)
		{
			this->sctpAssociation->ResetStreams(
			  std::array<uint16_t, 1>{ dataConsumer->GetSctpStreamParameters().streamId });
		}

		// Notify the listener.
		this->listener->OnTransportDataConsumerDataProducerClosed(this, dataConsumer);

		MS_DEBUG_DEV("DataConsumer closed [dataConsumerId:%s]", dataConsumer->id.c_str());

		// Delete it.
		delete dataConsumer;
	}

	bool Transport::OnAssociationSendData(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		// Ignore if destroying.
		// NOTE: This is because when the child class (i.e. WebRtcTransport) is deleted,
		// its destructor is called first and then the parent Transport's destructor,
		// and we would end here calling SendData() which is an abstract method.
		if (this->isDestroying)
		{
			MS_WARN_DEV("ignoring sending data because Transport is being destroying");

			return false;
		}

		return SendData(data, len);
	}

	void Transport::OnAssociationConnecting()
	{
		MS_TRACE();

		// Notify the Node Transport.
		auto sctpStateChangeNotification = FBS::Transport::CreateSctpStateChangeNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::SctpAssociation::SctpState::CONNECTING);

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::TRANSPORT_SCTP_STATE_CHANGE,
		  FBS::Notification::Body::Transport_SctpStateChangeNotification,
		  sctpStateChangeNotification);
	}

	void Transport::OnAssociationConnected()
	{
		MS_TRACE();

		// Notify the upper layer.

		// First tell it about the SCTP negotiated capabilities.
		auto sctpNegotiatedCapabilitiesOffset = FBS::SctpAssociation::CreateSctpNegotiatedCapabilities(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  this->sctpAssociation->GetNegotiatedMaxOutboundStreams(),
		  this->sctpAssociation->GetNegotiatedMaxInboundStreams());

		auto sctpNegotiatedCapabilitiesNotification =
		  FBS::Transport::CreateSctpNegotiatedCapabilitiesNotification(
		    this->shared->GetChannelNotifier()->GetBufferBuilder(), sctpNegotiatedCapabilitiesOffset);

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::TRANSPORT_SCTP_NEGOTIATED_CAPABILITIES,
		  FBS::Notification::Body::Transport_SctpNegotiatedCapabilitiesNotification,
		  sctpNegotiatedCapabilitiesNotification);

		// Then announce "connected" SCTP state.
		auto sctpStateChangeNotification = FBS::Transport::CreateSctpStateChangeNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::SctpAssociation::SctpState::CONNECTED);

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::TRANSPORT_SCTP_STATE_CHANGE,
		  FBS::Notification::Body::Transport_SctpStateChangeNotification,
		  sctpStateChangeNotification);

// For debugging purposes.
#if MS_LOG_DEV_LEVEL == 3
		MS_DUMP("SCTP association connected:");
		this->sctpAssociation->Dump();
#endif
	}

	void Transport::OnAssociationFailed(RTC::SCTP::Types::ErrorKind errorKind, std::string_view errorMessage)
	{
		MS_TRACE();

		const auto errorKindStringView = RTC::SCTP::Types::errorKindToString(errorKind);

		if (errorKind == RTC::SCTP::Types::ErrorKind::SUCCESS || errorKind == RTC::SCTP::Types::ErrorKind::PEER_REPORTED)
		{
			MS_DEBUG_TAG(
			  sctp,
			  "SCTP association failed [errorKind:%.*s, message:%.*s]",
			  static_cast<int>(errorKindStringView.size()),
			  errorKindStringView.data(),
			  static_cast<int>(errorMessage.size()),
			  errorMessage.data());
		}
		else
		{
			MS_WARN_TAG(
			  sctp,
			  "SCTP association failed [errorKind:%.*s, message:%.*s]",
			  static_cast<int>(errorKindStringView.size()),
			  errorKindStringView.data(),
			  static_cast<int>(errorMessage.size()),
			  errorMessage.data());
		}

		// Tell all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			if (dataConsumer->GetType() == RTC::DataConsumer::Type::SCTP)
			{
				dataConsumer->SctpAssociationClosed();
			}
		}

		// Notify the Node Transport.
		auto sctpStateChangeNotification = FBS::Transport::CreateSctpStateChangeNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::SctpAssociation::SctpState::FAILED);

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::TRANSPORT_SCTP_STATE_CHANGE,
		  FBS::Notification::Body::Transport_SctpStateChangeNotification,
		  sctpStateChangeNotification);
	}

	void Transport::OnAssociationClosed(RTC::SCTP::Types::ErrorKind errorKind, std::string_view errorMessage)
	{
		MS_TRACE();

		const auto errorKindStringView = RTC::SCTP::Types::errorKindToString(errorKind);

		if (errorKind == RTC::SCTP::Types::ErrorKind::SUCCESS || errorKind == RTC::SCTP::Types::ErrorKind::PEER_REPORTED)
		{
			MS_DEBUG_TAG(
			  sctp,
			  "SCTP association closed [errorKind:%.*s, message:%.*s]",
			  static_cast<int>(errorKindStringView.size()),
			  errorKindStringView.data(),
			  static_cast<int>(errorMessage.size()),
			  errorMessage.data());
		}
		else
		{
			MS_WARN_TAG(
			  sctp,
			  "SCTP association closed [errorKind:%.*s, message:%.*s]",
			  static_cast<int>(errorKindStringView.size()),
			  errorKindStringView.data(),
			  static_cast<int>(errorMessage.size()),
			  errorMessage.data());
		}

		// Tell all DataConsumers.
		for (auto& kv : this->mapDataConsumers)
		{
			auto* dataConsumer = kv.second;

			if (dataConsumer->GetType() == RTC::DataConsumer::Type::SCTP)
			{
				dataConsumer->SctpAssociationClosed();
			}
		}

		// Notify the Node Transport.
		auto sctpStateChangeNotification = FBS::Transport::CreateSctpStateChangeNotification(
		  this->shared->GetChannelNotifier()->GetBufferBuilder(),
		  FBS::SctpAssociation::SctpState::CLOSED);

		this->shared->GetChannelNotifier()->Emit(
		  this->id,
		  FBS::Notification::Event::TRANSPORT_SCTP_STATE_CHANGE,
		  FBS::Notification::Body::Transport_SctpStateChangeNotification,
		  sctpStateChangeNotification);
	}

	void Transport::OnAssociationRestarted()
	{
		MS_TRACE();

		MS_DEBUG_TAG(sctp, "SCTP association restarted");
	}

	void Transport::OnAssociationError(RTC::SCTP::Types::ErrorKind errorKind, std::string_view errorMessage)
	{
		MS_TRACE();

		const auto errorKindStringView = RTC::SCTP::Types::errorKindToString(errorKind);

		MS_WARN_TAG(
		  sctp,
		  "SCTP association error [errorKind:%.*s, message:%.*s]",
		  static_cast<int>(errorKindStringView.size()),
		  errorKindStringView.data(),
		  static_cast<int>(errorMessage.size()),
		  errorMessage.data());
	}

	void Transport::OnAssociationMessageReceived(RTC::SCTP::Message message)
	{
		MS_TRACE();

		RTC::DataProducer* dataProducer = this->sctpListener.GetDataProducer(message.GetStreamId());

		if (!dataProducer)
		{
			MS_WARN_2TAGS(
			  sctp,
			  message,
			  "no suitable DataProducer for received SCTP message [streamId:%" PRIu16 "]",
			  message.GetStreamId());

			return;
		}

		// Pass the SCTP message to the corresponding DataProducer.
		try
		{
			std::vector<uint16_t> subchannels;
			std::optional<uint16_t> requiredSubchannel;
			std::optional<uint16_t> ignoredSubchannel;

			// When this is a pipe transport, the subchannels, required subchannel and
			// ignored subchannel may be encoded at the beginning of the message payload.
			if (this->IsPipe())
			{
				RTC::SubchannelsCodec::DecodeSubchannels(
				  message, subchannels, requiredSubchannel, ignoredSubchannel);
			}

			dataProducer->ReceiveMessage(
			  std::move(message), subchannels, requiredSubchannel, ignoredSubchannel);
		}
		catch (std::exception& error)
		{
			MS_WARN_2TAGS(
			  sctp,
			  message,
			  "DataProducer::ReceiveMessage() failed for received SCTP message [streamId:%" PRIu16 "]: %s",
			  message.GetStreamId(),
			  error.what());
		}
	}

	void Transport::OnAssociationStreamsResetPerformed(std::span<const uint16_t> /*outboundStreamIds*/)
	{
		MS_TRACE();

		MS_DEBUG_DEV("SCTP association streams reset performed");
	}

	void Transport::OnAssociationStreamsResetFailed(
	  std::span<const uint16_t> /*outboundStreamIds*/, std::string_view errorMessage)
	{
		MS_TRACE();

		MS_WARN_TAG(
		  sctp,
		  "SCTP association streams reset failed [message:%.*s]",
		  static_cast<int>(errorMessage.size()),
		  errorMessage.data());
	}

	void Transport::OnAssociationInboundStreamsReset(std::span<const uint16_t> inboundStreamIds)
	{
		MS_TRACE();

		// https://datatracker.ietf.org/doc/html/rfc8831#section-6.7
		//
		// "Closing of a data channel MUST be signaled by resetting the corresponding
		// outgoing streams [RFC6525]. This means that if one side decides to close
		// the data channel, it resets the corresponding outgoing stream. When the
		// peer sees that an incoming stream was reset, it also resets its
		// corresponding outgoing stream."
		if (this->sctpAssociation->IsDataChannel())
		{
			std::vector<RTC::DataConsumer*> dataConsumersToClose;
			std::vector<uint16_t> streamsToReset;

			for (const auto streamId : inboundStreamIds)
			{
				// Only reset the outgoing stream if there is a live DataConsumer
				// using it. If the DataChannel was closed by the app, the DataConsumer
				// will have already been closed and removed.
				const auto it = this->mapSctpStreamIdDataConsumers.find(streamId);

				if (it != this->mapSctpStreamIdDataConsumers.end())
				{
					auto* dataConsumer = it->second;

					dataConsumersToClose.push_back(dataConsumer);
					streamsToReset.push_back(streamId);
				}
			}

			if (!dataConsumersToClose.empty())
			{
				for (auto* dataConsumer : dataConsumersToClose)
				{
					// Remove it from the maps.
					this->mapDataConsumers.erase(dataConsumer->id);

					if (dataConsumer->GetType() == RTC::DataConsumer::Type::SCTP)
					{
						this->mapSctpStreamIdDataConsumers.erase(dataConsumer->GetSctpStreamParameters().streamId);
					}

					// Notify the listener.
					this->listener->OnTransportDataConsumerClosed(this, dataConsumer);

					MS_DEBUG_DEV(
					  "SCTP DataConsumer closed via SCTP inbound stream reset [dataConsumerId:%s, streamId:%" PRIu16
					  "]",
					  dataConsumer->id.c_str(),
					  dataConsumer->GetSctpStreamParameters().streamId);

					// Delete it.
					delete dataConsumer;
				}

				// NOTE: This must be called after removing data consumers from the maps,
				// otherwise if `OnAssociationStreamBufferedAmountLow()` was triggered it
				// would end up emitting an event associated to an already closed data
				// consumer.
				this->sctpAssociation->ResetStreams(streamsToReset);
			}
		}
	}

	void Transport::OnAssociationStreamBufferedAmountLow(uint16_t streamId)
	{
		MS_TRACE();

		const auto* dataConsumer = GetSctpDataConsumerByStreamId(streamId);

		if (!dataConsumer)
		{
			return;
		}

		dataConsumer->SctpBufferedAmountLow(this->sctpAssociation->GetStreamBufferedAmount(streamId));
	}

	void Transport::OnAssociationTotalBufferedAmountLow()
	{
		MS_TRACE();

		// TODO: SCTP: Here we should emit a new event to the upper layer saying
		// that the transport SCTP total buffered amount is low. However we don't
		// expose `SctpOptions::totalBufferedAmountLowThreshold` to Transport.
	}

	bool Transport::OnAssociationIsTransportReadyForSctp()
	{
		MS_TRACE();

		// We are ready for SCTP traffic if the transport is connected (e.g. the
		// WebRtcTransport has ICE and DTLS connected) and there is at least a
		// DataProducer or DataConsumer.
		//
		// NOTE: We don't want to start SCTP connection if there are no DataProducers
		// and DataConsumers because the peer (e.g. a browser) may have not started
		// its SCTP stack (e.g. no "m=application" media section in its SDP) so if we
		// initiate the SCTP connection it would fail after some time.
		return IsConnected() && (!this->mapDataProducers.empty() || !this->mapDataConsumers.empty());
	}

	void Transport::OnRttCalculatorRttUpdated(RTC::RttCalculator* /*rttCalculator*/, int64_t avgRttMs)
	{
		MS_TRACE();

		if (this->senderTransportCongestionController)
		{
			this->senderTransportCongestionController->OnRttUpdate(avgRttMs * 1000);
		}

		if (this->receiverTransportCongestionController)
		{
			this->receiverTransportCongestionController->OnRttUpdate(avgRttMs * 1000);
		}
	}

	void Transport::OnSenderTransportCongestionControllerTargetBitrate(
	  RTC::BWE::SenderTransportCongestionController* /*senderTransportCongestionController*/,
	  int64_t targetBitrate)
	{
		MS_TRACE();

		MS_DEBUG_TAG(bwe, "outgoing available bitrate [targetBitrate:%" PRIi64 "]", targetBitrate);

		DistributeAvailableOutgoingBitrate();
		ComputeOutgoingDesiredBitrate();

		// May emit 'trace' event.
		EmitTraceEventBweType();
	}

	void Transport::OnSenderTransportCongestionControllerSendRtpProbePacket(
	  RTC::BWE::SenderTransportCongestionController* /*senderTransportCongestionController*/,
	  RTC::RTP::Packet* packet,
	  std::optional<int64_t> sequenceNumber)
	{
		MS_TRACE();

		// NOTE: The probe packet already carries both extensions written and comes
		// with the sequence number it was given, if any, so it must not be handed to
		// OnRtpPacketToBeSent() again.

#ifdef MS_RTC_LOGGER_SEND_BURST
		this->sendBurstLogger.Sent(
		  this->shared->GetLoopTimeMs(),
		  packet->GetLength(),
		  /*isRetransmission*/ false,
		  /*isProbation*/ true);
#endif

		// May emit 'trace' event.
		EmitTraceEventProbationType(packet);

		const size_t size = packet->GetLength();

		const std::weak_ptr<RTC::BWE::SenderTransportCongestionController>
		  senderTransportCongestionControllerWeakPtr(this->senderTransportCongestionController);

		auto* shared = this->shared;

		SendRtpPacket(
		  nullptr,
		  packet,
		  [senderTransportCongestionControllerWeakPtr, shared, sequenceNumber, size](bool sent) -> void
		  {
			  if (sent)
			  {
				  auto senderTransportCongestionController =
				    senderTransportCongestionControllerWeakPtr.lock();

				  if (senderTransportCongestionController)
				  {
					  senderTransportCongestionController->OnRtpPacketSent(
					    sequenceNumber, size, shared->GetTimeUs());
				  }
			  }
		  });

		this->sendProbationTransmission.Update(packet);

		MS_DEBUG_DEV(
		  "probation sent [seq:%" PRIu16 ", wideSeq:%" PRIi64 ", size:%zu, bitrate:%" PRIi64 "]",
		  packet->GetSequenceNumber(),
		  sequenceNumber.value_or(-1),
		  packet->GetLength(),
		  this->sendProbationTransmission.GetBitrate(this->shared->GetTimeMs()).value_or(0));
	}

	void Transport::OnReceiverTransportCongestionControllerSendRtcpPacket(
	  RTC::BWE::ReceiverTransportCongestionController* /*receiverTransportCongestionController*/,
	  RTC::RTCP::Packet* packet)
	{
		MS_TRACE();

		packet->Serialize(RTC::RTCP::SerializationBuffer);

		SendRtcpPacket(packet);
	}

	void Transport::OnTimer(TimerHandleInterface* timer)
	{
		MS_TRACE();

		// RTCP timer.
		if (timer == this->rtcpTimer.get())
		{
			auto intervalMs = static_cast<int64_t>(RTC::RTCP::MaxVideoIntervalMs);

			SendRtcp(this->shared->GetTimeUs());

			/*
			 * The interval between RTCP packets is varied randomly over the range
			 * [1.0, 1.5] times the calculated interval to avoid unintended
			 * synchronization of all participants.
			 */
			intervalMs = (intervalMs * Utils::Crypto::GetRandomUInt<uint16_t>(10, 15)) / 10;

			this->rtcpTimer->Start(intervalMs);
		}
#ifdef MS_RTC_LOGGER_SEND_BURST
		else if (timer == this->sendBurstLoggerTimer.get())
		{
			this->sendBurstLogger.Log(this->shared->GetLoopTimeMs());
		}
#endif
	}
} // namespace RTC
