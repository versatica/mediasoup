use crate::data_structures::{AppData, EventDirection, ProducerInternal, TransportInternal};
use crate::messages::{
    TransportDumpRequest, TransportEnableTraceEventRequest, TransportEnableTraceEventRequestData,
    TransportGetStatsRequest, TransportProduceRequest, TransportProduceRequestData,
    TransportSetMaxIncomingBitrateData, TransportSetMaxIncomingBitrateRequest,
};
use crate::ortc::{RtpParametersError, RtpParametersMappingError};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::Router;
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
pub trait Transport {
    /// Transport id.
    fn id(&self) -> TransportId;

    /// App custom data.
    fn app_data(&self) -> &AppData;

    /// Set maximum incoming bitrate for receiving media.
    async fn set_max_incoming_bitrate(&self, bitrate: u32) -> Result<(), RequestError>;

    /// Create a Producer.
    ///
    /// Transport will be kept alive as long as at least one producer instance is alive.
    async fn produce(&self, producer_options: ProducerOptions) -> Result<Producer, ProduceError>;
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

    fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F);
}

#[derive(Debug, Error)]
pub enum ProduceError {
    #[error("Producer with ID {0} already exists")]
    AlreadyExists(ProducerId),
    #[error("Incorrect RTP parameters: {0}")]
    IncorrectRtpParameters(RtpParametersError),
    #[error("RTP mapping error: {0}")]
    FailedRtpParametersMapping(RtpParametersMappingError),
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

    fn has_producer(&self, id: &ProducerId) -> bool;

    fn executor(&self) -> &Arc<Executor>;

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
            if self.has_producer(id) {
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
            ortc::get_producer_rtp_parameters_mapping(&rtp_parameters, &router_rtp_capabilities)
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
            self.router().id(),
            Arc::clone(self.executor()),
            self.channel().clone(),
            self.payload_channel().clone(),
            app_data,
            Box::new(self.clone()),
        );

        Ok(producer_fut.await)
    }
}
