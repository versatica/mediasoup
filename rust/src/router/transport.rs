use crate::consumer::{Consumer, ConsumerId, ConsumerOptions};
use crate::data_consumer::{DataConsumer, DataConsumerId, DataConsumerOptions, DataConsumerType};
use crate::data_producer::{DataProducer, DataProducerId, DataProducerOptions, DataProducerType};
use crate::data_structures::{AppData, EventDirection};
use crate::messages::{
    ConsumerInternal, DataConsumerInternal, DataProducerInternal, ProducerInternal,
    TransportConsumeData, TransportConsumeDataData, TransportConsumeDataRequest,
    TransportConsumeRequest, TransportDumpRequest, TransportEnableTraceEventData,
    TransportEnableTraceEventRequest, TransportGetStatsRequest, TransportInternal,
    TransportProduceData, TransportProduceDataData, TransportProduceDataRequest,
    TransportProduceRequest, TransportSetMaxIncomingBitrateData,
    TransportSetMaxIncomingBitrateRequest,
};
use crate::ortc::{
    ConsumerRtpParametersError, RtpCapabilitiesError, RtpParametersError, RtpParametersMappingError,
};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::{Router, RouterId};
use crate::rtp_parameters::RtpEncodingParameters;
use crate::worker::{Channel, PayloadChannel, RequestError};
use crate::{ortc, uuid_based_wrapper_type};
use async_executor::Executor;
use async_mutex::Mutex as AsyncMutex;
use async_trait::async_trait;
use event_listener_primitives::HandlerId;
use log::*;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::fmt::Debug;
use std::future::Future;
use std::marker::PhantomData;
use std::pin::Pin;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use thiserror::Error;
use uuid::Uuid;

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

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RtpListener {
    /// Map from Ssrc (as string) to producer ID
    pub mid_table: HashMap<String, ProducerId>,
    /// Map from Ssrc (as string) to producer ID
    pub rid_table: HashMap<String, ProducerId>,
    /// Map from Ssrc (as string) to producer ID
    pub ssrc_table: HashMap<String, ProducerId>,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RecvRtpHeaderExtensions {
    mid: Option<u8>,
    rid: Option<u8>,
    rrid: Option<u8>,
    abs_send_time: Option<u8>,
    transport_wide_cc01: Option<u8>,
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct SctpListener {
    /// Map from stream ID (as string) to data producer ID
    stream_id_table: HashMap<String, DataProducerId>,
}

pub(super) enum TransportType {
    Direct,
    Pipe,
    Plain,
    WebRtc,
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
    ) -> Result<DataProducer, ProduceDataError>;

    /// Create a DataConsumer.
    ///
    /// Transport will be kept alive as long as at least one data consumer instance is alive.
    async fn consume_data(
        &self,
        data_consumer_options: DataConsumerOptions,
    ) -> Result<DataConsumer, ConsumeDataError>;
}

#[async_trait(?Send)]
pub trait TransportGeneric<Dump, Stat>: Transport + Clone {
    /// Dump Transport.
    async fn dump(&self) -> Result<Dump, RequestError>;

    /// Get Transport stats.
    async fn get_stats(&self) -> Result<Vec<Stat>, RequestError>;

    async fn enable_trace_event(
        &self,
        types: Vec<TransportTraceEventType>,
    ) -> Result<(), RequestError>;

    fn on_new_producer<F: Fn(&Producer) + Send + 'static>(&self, callback: F)
        -> HandlerId<'static>;

    fn on_new_consumer<F: Fn(&Consumer) + Send + 'static>(&self, callback: F)
        -> HandlerId<'static>;

    fn on_new_data_producer<F: Fn(&DataProducer) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static>;

    fn on_new_data_consumer<F: Fn(&DataConsumer) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static>;

    fn on_trace<F: Fn(&TransportTraceEventData) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static>;

    fn on_router_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId<'static>;

    fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId<'static>;
}

#[derive(Debug, Error, Eq, PartialEq)]
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

#[derive(Debug, Error, Eq, PartialEq)]
pub enum ConsumeError {
    #[error("Producer with id \"{0}\" not found")]
    ProducerNotFound(ProducerId),
    #[error("RTP capabilities error: {0}")]
    FailedRtpCapabilitiesValidation(RtpCapabilitiesError),
    #[error("Bad consumer RTP parameters: {0}")]
    BadConsumerRtpParameters(ConsumerRtpParametersError),
    #[error("Request to worker failed: {0}")]
    Request(RequestError),
}

#[derive(Debug, Error, Eq, PartialEq)]
pub enum ProduceDataError {
    #[error("Data producer with the same id \"{0}\" already exists")]
    AlreadyExists(DataProducerId),
    #[error("SCTP stream parameters are required for this transport")]
    SctpStreamParametersRequired,
    #[error("Request to worker failed: {0}")]
    Request(RequestError),
}

#[derive(Debug, Error, Eq, PartialEq)]
pub enum ConsumeDataError {
    #[error("Data producer with id \"{0}\" not found")]
    DataProducerNotFound(DataProducerId),
    #[error("no free sctp_stream_id available in transport")]
    NoSctpStreamId,
    #[error("Request to worker failed: {0}")]
    Request(RequestError),
}

#[async_trait(?Send)]
pub(super) trait TransportImpl<Dump, Stat>
where
    Dump: Debug + DeserializeOwned + 'static,
    Stat: Debug + DeserializeOwned + 'static,
    Self: Transport + TransportGeneric<Dump, Stat> + 'static,
{
    fn router(&self) -> &Router;

    fn channel(&self) -> &Channel;

    fn payload_channel(&self) -> &PayloadChannel;

    fn executor(&self) -> &Arc<Executor<'static>>;

    fn next_mid_for_consumers(&self) -> &AtomicUsize;

    fn used_sctp_stream_ids(&self) -> &AsyncMutex<HashMap<u16, bool>>;

    fn cname_for_producers(&self) -> &AsyncMutex<Option<String>>;

    async fn allocate_sctp_stream_id(&self) -> Option<u16> {
        let mut used_sctp_stream_ids = self.used_sctp_stream_ids().lock().await;
        // This is simple, but not the fastest implementation, maybe worth improving
        for (index, used) in used_sctp_stream_ids.iter_mut() {
            if !*used {
                *used = true;
                return Some(*index);
            }
        }

        None
    }

    fn deallocate_sctp_stream_id(
        &self,
        sctp_stream_id: u16,
    ) -> Pin<Box<dyn Future<Output = ()> + Send + '_>> {
        async fn deallocate_sctp_stream_id(
            used_sctp_stream_ids: &AsyncMutex<HashMap<u16, bool>>,
            sctp_stream_id: u16,
        ) {
            if let Some(used) = used_sctp_stream_ids.lock().await.get_mut(&sctp_stream_id) {
                *used = false;
            }
        }
        let used_sctp_stream_ids = self.used_sctp_stream_ids();
        Box::pin(deallocate_sctp_stream_id(
            used_sctp_stream_ids,
            sctp_stream_id,
        ))
    }

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
                data: TransportEnableTraceEventData { types },
            })
            .await
    }

    async fn produce_impl(
        &self,
        producer_options: ProducerOptions,
        transport_type: TransportType,
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

        // Don't do this in PipeTransports since there we must keep CNAME value in each Producer.
        if !matches!(transport_type, TransportType::Pipe) {
            let mut cname_for_producers = self.cname_for_producers().lock().await;
            if let Some(cname_for_producers) = cname_for_producers.as_ref() {
                rtp_parameters.rtcp.cname = Some(cname_for_producers.clone());
            } else if let Some(cname) = rtp_parameters.rtcp.cname.as_ref() {
                // If CNAME is given and we don't have yet a CNAME for Producers in this
                // Transport, take it.
                cname_for_producers.replace(cname.clone());
            } else {
                // Otherwise if we don't have yet a CNAME for Producers and the RTP parameters
                // do not include CNAME, create a random one.
                let cname = Uuid::new_v4().to_string();
                cname_for_producers.replace(cname.clone());

                // Override Producer's CNAME.
                rtp_parameters.rtcp.cname = Some(cname);
            }
        }

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

        let producer_id = id.unwrap_or_else(ProducerId::new);

        let response = self
            .channel()
            .request(TransportProduceRequest {
                internal: ProducerInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                    producer_id,
                },
                data: TransportProduceData {
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
            response.r#type,
            rtp_parameters,
            consumable_rtp_parameters,
            paused,
            Arc::clone(self.executor()),
            self.channel().clone(),
            self.payload_channel().clone(),
            app_data,
            self.clone(),
            matches!(transport_type, TransportType::Direct),
        );

        Ok(producer_fut.await)
    }

    async fn consume_impl(
        &self,
        consumer_options: ConsumerOptions,
        transport_type: TransportType,
        rtx: bool,
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
        let rtp_parameters = if matches!(transport_type, TransportType::Pipe) {
            ortc::get_pipe_consumer_rtp_parameters(producer.consumable_rtp_parameters(), rtx)
        } else {
            let mut rtp_parameters = ortc::get_consumer_rtp_parameters(
                producer.consumable_rtp_parameters(),
                rtp_capabilities,
            )
            .map_err(ConsumeError::BadConsumerRtpParameters)?;

            // We use up to 8 bytes for MID (string).
            let mid = self.next_mid_for_consumers().fetch_add(1, Ordering::AcqRel) % 100_000_000;

            // Set MID.
            rtp_parameters.mid = Some(format!("{}", mid));

            rtp_parameters
        };

        let consumer_id = ConsumerId::new();

        let r#type = producer.r#type().into();

        let response = self
            .channel()
            .request(TransportConsumeRequest {
                internal: ConsumerInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                    consumer_id,
                    producer_id: producer.id(),
                },
                data: TransportConsumeData {
                    kind: producer.kind(),
                    rtp_parameters: rtp_parameters.clone(),
                    r#type,
                    consumable_rtp_encodings: producer
                        .consumable_rtp_parameters()
                        .encodings
                        .clone(),
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
            response.paused,
            Arc::clone(self.executor()),
            self.channel().clone(),
            self.payload_channel().clone(),
            response.producer_paused,
            response.score,
            response.preferred_layers,
            app_data,
            self.clone(),
        );

        Ok(consumer_fut.await)
    }

    async fn produce_data_impl(
        &self,
        r#type: DataProducerType,
        data_producer_options: DataProducerOptions,
        transport_type: TransportType,
    ) -> Result<DataProducer, ProduceDataError> {
        if let Some(id) = &data_producer_options.id {
            if self.router().has_data_producer(id) {
                return Err(ProduceDataError::AlreadyExists(*id));
            }
        }

        match r#type {
            DataProducerType::Sctp => {
                if data_producer_options.sctp_stream_parameters.is_none() {
                    return Err(ProduceDataError::SctpStreamParametersRequired);
                }
            }
            DataProducerType::Direct => {
                if data_producer_options.sctp_stream_parameters.is_some() {
                    warn!(
                        "sctp_stream_parameters are ignored when producing data on a DirectTransport",
                    );
                }
            }
        }

        let DataProducerOptions {
            id,
            sctp_stream_parameters,
            label,
            protocol,
            app_data,
        } = data_producer_options;

        let data_producer_id = id.unwrap_or_else(DataProducerId::new);

        let response = self
            .channel()
            .request(TransportProduceDataRequest {
                internal: DataProducerInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                    data_producer_id,
                },
                data: TransportProduceDataData {
                    r#type,
                    sctp_stream_parameters,
                    label,
                    protocol,
                },
            })
            .await
            .map_err(ProduceDataError::Request)?;

        let data_producer_fut = DataProducer::new(
            data_producer_id,
            response.r#type,
            response.sctp_stream_parameters,
            response.label,
            response.protocol,
            Arc::clone(self.executor()),
            self.channel().clone(),
            self.payload_channel().clone(),
            app_data,
            self.clone(),
            matches!(transport_type, TransportType::Direct),
        );

        Ok(data_producer_fut.await)
    }

    async fn consume_data_impl(
        &self,
        r#type: DataConsumerType,
        data_consumer_options: DataConsumerOptions,
        transport_type: TransportType,
    ) -> Result<DataConsumer, ConsumeDataError> {
        let DataConsumerOptions {
            data_producer_id,
            ordered,
            max_packet_life_time,
            max_retransmits,
            app_data,
        } = data_consumer_options;

        let data_producer = match self.router().get_data_producer(&data_producer_id) {
            Some(data_producer) => data_producer,
            None => {
                return Err(ConsumeDataError::DataProducerNotFound(data_producer_id));
            }
        };

        let sctp_stream_parameters = match r#type {
            DataConsumerType::Sctp => {
                let mut sctp_stream_parameters = data_producer.sctp_stream_parameters();
                if let Some(sctp_stream_parameters) = &mut sctp_stream_parameters {
                    if let Some(stream_id) = self.allocate_sctp_stream_id().await {
                        sctp_stream_parameters.stream_id = stream_id;
                    } else {
                        return Err(ConsumeDataError::NoSctpStreamId);
                    }
                    if let Some(ordered) = ordered {
                        sctp_stream_parameters.ordered = ordered;
                    }
                    if let Some(max_packet_life_time) = max_packet_life_time {
                        sctp_stream_parameters.max_packet_life_time = Some(max_packet_life_time);
                    }
                    if let Some(max_retransmits) = max_retransmits {
                        sctp_stream_parameters.max_retransmits = Some(max_retransmits);
                    }
                }
                sctp_stream_parameters
            }
            DataConsumerType::Direct => {
                if ordered.is_some() || max_packet_life_time.is_some() || max_retransmits.is_some()
                {
                    warn!("ordered, max_packet_life_time and max_retransmits are ignored when consuming data on a DirectTransport");
                }
                None
            }
        };

        let data_consumer_id = DataConsumerId::new();

        let response = self
            .channel()
            .request(TransportConsumeDataRequest {
                internal: DataConsumerInternal {
                    router_id: self.router().id(),
                    transport_id: self.id(),
                    data_producer_id: data_producer.id(),
                    data_consumer_id,
                },
                data: TransportConsumeDataData {
                    r#type,
                    sctp_stream_parameters,
                    label: data_producer.label().clone(),
                    protocol: data_producer.protocol().clone(),
                },
            })
            .await
            .map_err(ConsumeDataError::Request)?;

        let data_consumer_fut = DataConsumer::new(
            data_consumer_id,
            response.r#type,
            response.sctp_stream_parameters,
            response.label,
            response.protocol,
            data_producer.id(),
            Arc::clone(self.executor()),
            self.channel().clone(),
            self.payload_channel().clone(),
            app_data,
            self.clone(),
            matches!(transport_type, TransportType::Direct),
        );

        let data_consumer = data_consumer_fut.await;

        if let Some(sctp_stream_parameters) = data_consumer.sctp_stream_parameters() {
            let stream_id = sctp_stream_parameters.stream_id;
            let transport = self.clone();
            let executor = Arc::clone(self.executor());
            data_consumer
                .on_close(move || {
                    executor
                        .spawn(async move {
                            transport.deallocate_sctp_stream_id(stream_id).await;
                        })
                        .detach();
                })
                .detach();
        }

        Ok(data_consumer)
    }
}
