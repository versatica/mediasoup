use crate::consumer::{Consumer, ConsumerId, ConsumerOptions};
use crate::data_producer::{DataProducer, DataProducerOptions};
use crate::data_structures::{AppData, EventDirection};
use crate::messages::{
    ConsumerInternal, ProducerInternal, TransportConsumeRequest, TransportConsumeRequestData,
    TransportDumpRequest, TransportEnableTraceEventRequest, TransportEnableTraceEventRequestData,
    TransportGetStatsRequest, TransportInternal, TransportProduceRequest,
    TransportProduceRequestData, TransportSetMaxIncomingBitrateData,
    TransportSetMaxIncomingBitrateRequest,
};
use crate::ortc::{
    ConsumerRtpParametersError, RouterRtpCapabilitiesError, RtpParametersError,
    RtpParametersMappingError,
};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::{Router, RouterId};
use crate::rtp_parameters::RtpEncodingParameters;
use crate::worker::{Channel, RequestError};
use crate::{ortc, uuid_based_wrapper_type};
use async_executor::Executor;
use async_trait::async_trait;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fmt::Debug;
use std::marker::PhantomData;
use std::sync::Arc;
use thiserror::Error;

uuid_based_wrapper_type!(TransportId);

#[derive(Debug, Clone, Deserialize, Serialize)]
#[serde(tag = "type", rename_all = "lowercase")]
pub enum TransportTraceEventData {
    Probation {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: EventDirection,
        // TODO: Clarify value structure
        /// Per type information.
        info: Value,
    },
    BWE {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: EventDirection,
        // TODO: Clarify value structure
        /// Per type information.
        info: Value,
    },
}

/// Valid types for 'trace' event.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum TransportTraceEventType {
    Probation,
    BWE,
}

#[async_trait(?Send)]
pub trait Transport
where
    Self: Send + Sync,
{
    /// Transport id.
    fn id(&self) -> TransportId;

    /// Router id.
    fn router_id(&self) -> RouterId;

    /// App custom data.
    fn app_data(&self) -> &AppData;

    /// Set maximum incoming bitrate for receiving media.
    async fn set_max_incoming_bitrate(&self, bitrate: u32) -> Result<(), RequestError>;

    /// Create a Producer.
    ///
    /// Transport will be kept alive as long as at least one producer instance is alive.
    async fn produce(&self, producer_options: ProducerOptions) -> Result<Producer, ProduceError>;

    /// Create a Consumer.
    ///
    /// Transport will be kept alive as long as at least one consumer instance is alive.
    async fn consume(&self, consumer_options: ConsumerOptions) -> Result<Consumer, ConsumeError>;

    /// Create a DataProducer.
    ///
    /// Transport will be kept alive as long as at least one data producer instance is alive.
    async fn produce_data(
        &self,
        data_producer_options: DataProducerOptions,
    ) -> Result<DataProducer, ProduceError>;
    // TODO
}

#[async_trait(?Send)]
pub trait TransportGeneric<Dump, Stat, RemoteParameters>: Transport {
    /// Dump Transport.
    async fn dump(&self) -> Result<Dump, RequestError>;

    /// Get Transport stats.
    async fn get_stats(&self) -> Result<Vec<Stat>, RequestError>;

    /// Provide the Transport remote parameters.
    async fn connect(&self, remote_parameters: RemoteParameters) -> Result<(), RequestError>;

    async fn enable_trace_event(
        &self,
        types: Vec<TransportTraceEventType>,
    ) -> Result<(), RequestError>;

    fn connect_new_producer<F: Fn(&Producer) + Send + 'static>(&self, callback: F);

    fn connect_trace<F: Fn(&TransportTraceEventData) + Send + 'static>(&self, callback: F);

    fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F);
}

#[derive(Debug, Error)]
pub enum ProduceError {
    #[error("Producer with the same id \"{0}\" already exists")]
    AlreadyExists(ProducerId),
    #[error("Incorrect RTP parameters: {0}")]
    IncorrectRtpParameters(RtpParametersError),
    #[error("RTP mapping error: {0}")]
    FailedRtpParametersMapping(RtpParametersMappingError),
    #[error("Request to worker failed: {0}")]
    Request(RequestError),
}

#[derive(Debug, Error)]
pub enum ConsumeError {
    #[error("Producer with id \"{0}\" not found")]
    ProducerNotFound(ProducerId),
    #[error("RTP capabilities error: {0}")]
    FailedRtpCapabilitiesValidation(RouterRtpCapabilitiesError),
    #[error("Bad consumer RTP parameters: {0}")]
    BadConsumerRtpParameters(ConsumerRtpParametersError),
    #[error("Request to worker failed: {0}")]
    Request(RequestError),
}

#[async_trait(?Send)]
pub(super) trait TransportImpl<Dump, Stat, RemoteParameters>
where
    Dump: Debug + DeserializeOwned + 'static,
    Stat: Debug + DeserializeOwned + 'static,
    RemoteParameters: 'static,
    Self: Transport + Clone + 'static,
{
    fn router(&self) -> &Router;

    fn channel(&self) -> &Channel;

    fn payload_channel(&self) -> &Channel;

    fn executor(&self) -> &Arc<Executor<'static>>;

    fn next_mid_for_consumers(&self) -> usize;

    async fn dump_impl(&self) -> Result<Dump, RequestError> {
        self.channel()
            .request(TransportDumpRequest {
                internal: TransportInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                },
                phantom_data: PhantomData {},
            })
            .await
    }

    async fn get_stats_impl(&self) -> Result<Vec<Stat>, RequestError> {
        self.channel()
            .request(TransportGetStatsRequest {
                internal: TransportInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                },
                phantom_data: PhantomData {},
            })
            .await
    }

    async fn set_max_incoming_bitrate_impl(&self, bitrate: u32) -> Result<(), RequestError> {
        self.channel()
            .request(TransportSetMaxIncomingBitrateRequest {
                internal: TransportInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                },
                data: TransportSetMaxIncomingBitrateData { bitrate },
            })
            .await
    }

    async fn enable_trace_event_impl(
        &self,
        types: Vec<TransportTraceEventType>,
    ) -> Result<(), RequestError> {
        self.channel()
            .request(TransportEnableTraceEventRequest {
                internal: TransportInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                },
                data: TransportEnableTraceEventRequestData { types },
            })
            .await
    }

    async fn produce_impl(
        &self,
        producer_options: ProducerOptions,
    ) -> Result<Producer, ProduceError> {
        if let Some(id) = &producer_options.id {
            if self.router().has_producer(id) {
                return Err(ProduceError::AlreadyExists(*id));
            }
        }

        let ProducerOptions {
            id,
            kind,
            mut rtp_parameters,
            paused,
            key_frame_request_delay,
            app_data,
        } = producer_options;

        ortc::validate_rtp_parameters(&rtp_parameters)
            .map_err(ProduceError::IncorrectRtpParameters)?;

        if rtp_parameters.encodings.is_empty() {
            rtp_parameters
                .encodings
                .push(RtpEncodingParameters::default());
        }

        // TODO: Port this piece of code from TypeScript
        // // Don't do this in PipeTransports since there we must keep CNAME value in
        // // each Producer.
        // if (this.constructor.name !== 'PipeTransport')
        // {
        // 	// If CNAME is given and we don't have yet a CNAME for Producers in this
        // 	// Transport, take it.
        // 	if (!this._cnameForProducers && rtpParameters.rtcp && rtpParameters.rtcp.cname)
        // 	{
        // 		this._cnameForProducers = rtpParameters.rtcp.cname;
        // 	}
        // 	// Otherwise if we don't have yet a CNAME for Producers and the RTP parameters
        // 	// do not include CNAME, create a random one.
        // 	else if (!this._cnameForProducers)
        // 	{
        // 		this._cnameForProducers = uuidv4().substr(0, 8);
        // 	}
        //
        // 	// Override Producer's CNAME.
        // 	rtpParameters.rtcp = rtpParameters.rtcp || {};
        // 	rtpParameters.rtcp.cname = this._cnameForProducers;
        // }

        let router_rtp_capabilities = self.router().rtp_capabilities();

        let rtp_mapping =
            ortc::get_producer_rtp_parameters_mapping(&rtp_parameters, router_rtp_capabilities)
                .map_err(ProduceError::FailedRtpParametersMapping)?;

        let consumable_rtp_parameters = ortc::get_consumable_rtp_parameters(
            kind,
            &rtp_parameters,
            router_rtp_capabilities,
            &rtp_mapping,
        );

        let producer_id = id.unwrap_or_else(|| ProducerId::new());

        let status = self
            .channel()
            .request(TransportProduceRequest {
                internal: ProducerInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                    producer_id,
                },
                data: TransportProduceRequestData {
                    kind,
                    rtp_parameters: rtp_parameters.clone(),
                    rtp_mapping,
                    key_frame_request_delay,
                    paused,
                },
            })
            .await
            .map_err(ProduceError::Request)?;

        let producer_fut = Producer::new(
            producer_id,
            kind,
            status.r#type,
            rtp_parameters,
            consumable_rtp_parameters,
            paused,
            Arc::clone(self.executor()),
            self.channel().clone(),
            self.payload_channel().clone(),
            app_data,
            Box::new(self.clone()),
        );

        Ok(producer_fut.await)
    }

    async fn consume_impl(
        &self,
        consumer_options: ConsumerOptions,
    ) -> Result<Consumer, ConsumeError> {
        let ConsumerOptions {
            producer_id,
            rtp_capabilities,
            paused,
            preferred_layers,
            app_data,
        } = consumer_options;
        ortc::validate_rtp_capabilities(&rtp_capabilities)
            .map_err(ConsumeError::FailedRtpCapabilitiesValidation)?;

        let producer = match self.router().get_producer(&producer_id) {
            Some(producer) => producer,
            None => {
                return Err(ConsumeError::ProducerNotFound(producer_id));
            }
        };

        // TODO: Maybe RtpParametersFinalized would be a better fit here
        let rtp_parameters = {
            let mut rtp_parameters = ortc::get_consumer_rtp_parameters(
                producer.consumable_rtp_parameters(),
                rtp_capabilities,
            )
            .map_err(ConsumeError::BadConsumerRtpParameters)?;

            // We use up to 8 bytes for MID (string).
            let mid = self.next_mid_for_consumers() % 100_000_000;

            // Set MID.
            rtp_parameters.mid = Some(format!("{}", mid));

            rtp_parameters
        };

        let consumer_id = ConsumerId::new();

        let r#type = producer.r#type().into();

        let status = self
            .channel()
            .request(TransportConsumeRequest {
                internal: ConsumerInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                    consumer_id,
                    producer_id: producer.id(),
                },
                data: TransportConsumeRequestData {
                    kind: producer.kind(),
                    rtp_parameters: rtp_parameters.clone(),
                    r#type,
                    consumable_rtp_encodings: producer.consumable_rtp_parameters().encodings,
                    paused,
                    preferred_layers,
                },
            })
            .await
            .map_err(ConsumeError::Request)?;

        let consumer_fut = Consumer::new(
            consumer_id,
            producer.id(),
            producer.kind(),
            r#type,
            rtp_parameters,
            status.paused,
            Arc::clone(self.executor()),
            self.channel().clone(),
            self.payload_channel().clone(),
            status.producer_paused,
            status.score,
            status.preferred_layers,
            app_data,
            Box::new(self.clone()),
        );

        Ok(consumer_fut.await)
    }

    async fn produce_data_impl(
        &self,
        data_producer_options: DataProducerOptions,
    ) -> Result<DataProducer, ProduceError> {
        todo!()
    }
}
