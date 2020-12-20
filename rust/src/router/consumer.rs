use crate::data_structures::{AppData, EventDirection};
use crate::messages::{
    ConsumerCloseRequest, ConsumerDumpRequest, ConsumerEnableTraceEventData,
    ConsumerEnableTraceEventRequest, ConsumerGetStatsRequest, ConsumerInternal,
    ConsumerPauseRequest, ConsumerRequestKeyFrameRequest, ConsumerResumeRequest,
    ConsumerSetPreferredLayersRequest, ConsumerSetPriorityData, ConsumerSetPriorityRequest,
};
use crate::producer::{ProducerId, ProducerStat, ProducerType};
use crate::rtp_parameters::{MediaKind, MimeType, RtpCapabilities, RtpParameters};
use crate::transport::{Transport, TransportGeneric};
use crate::uuid_based_wrapper_type;
use crate::worker::{
    Channel, NotificationMessage, PayloadChannel, RequestError, SubscriptionHandler,
};
use async_executor::Executor;
use bytes::Bytes;
use event_listener_primitives::{Bag, HandlerId};
use log::*;
use parking_lot::Mutex as SyncMutex;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fmt::Debug;
use std::sync::Arc;

uuid_based_wrapper_type!(ConsumerId);

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ConsumerLayers {
    /// The spatial layer index (from 0 to N).
    pub spatial_layer: u8,
    /// The temporal layer index (from 0 to N).
    pub temporal_layer: Option<u8>,
}

#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ConsumerScore {
    /// The score of the RTP stream of the consumer.
    pub score: u8,
    /// The score of the currently selected RTP stream of the producer.
    pub producer_score: u8,
    /// The scores of all RTP streams in the producer ordered by encoding (just useful when the
    /// producer uses simulcast).
    pub producer_scores: Vec<u8>,
}

#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct ConsumerOptions {
    /// The id of the Producer to consume.
    pub producer_id: ProducerId,
    /// RTP capabilities of the consuming endpoint.
    pub rtp_capabilities: RtpCapabilities,
    /// Whether the Consumer must start in paused mode. Default false.
    ///
    /// When creating a video Consumer, it's recommended to set paused to true, then transmit the
    /// Consumer parameters to the consuming endpoint and, once the consuming endpoint has created
    /// its local side Consumer, unpause the server side Consumer using the resume() method. This is
    /// an optimization to make it possible for the consuming endpoint to render the video as far as
    /// possible. If the server side Consumer was created with paused: false, mediasoup will
    /// immediately request a key frame to the remote Producer and such a key frame may reach the
    /// consuming endpoint even before it's ready to consume it, generating “black” video until the
    /// device requests a keyframe by itself.
    pub paused: bool,
    /// Preferred spatial and temporal layer for simulcast or SVC media sources.
    /// If unset, the highest ones are selected.
    pub preferred_layers: Option<ConsumerLayers>,
    /// Custom application data.
    pub app_data: AppData,
}

impl ConsumerOptions {
    pub fn new(producer_id: ProducerId, rtp_capabilities: RtpCapabilities) -> Self {
        Self {
            producer_id,
            rtp_capabilities,
            paused: false,
            preferred_layers: None,
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RtpStreamParams {
    pub clock_rate: u32,
    pub cname: String,
    pub encoding_idx: usize,
    pub mime_type: MimeType,
    pub payload_type: u8,
    pub spatial_layers: u8,
    pub ssrc: u32,
    pub temporal_layers: u8,
    pub use_dtx: bool,
    pub use_in_band_fec: bool,
    pub use_nack: bool,
    pub use_pli: bool,
    pub rid: Option<String>,
    pub rtc_ssrc: Option<u32>,
    pub rtc_payload_type: Option<u8>,
}

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RtpStream {
    pub params: RtpStreamParams,
    pub score: u8,
}

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RtpRtxParameters {
    pub ssrc: Option<u32>,
}

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct ConsumableRtpEncoding {
    pub ssrc: Option<u32>,
    pub rid: Option<String>,
    pub codec_payload_type: Option<u8>,
    pub rtx: Option<RtpRtxParameters>,
    pub max_bitrate: Option<u32>,
    pub max_framerate: Option<f64>,
    pub dtx: Option<bool>,
    pub scalability_mode: Option<String>,
    pub spatial_layers: Option<u8>,
    pub temporal_layers: Option<u8>,
    pub ksvc: Option<bool>,
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
#[non_exhaustive]
pub struct ConsumerDump {
    pub id: ConsumerId,
    pub kind: MediaKind,
    pub paused: bool,
    pub priority: u8,
    pub producer_id: ProducerId,
    pub producer_paused: bool,
    pub rtp_parameters: RtpParameters,
    pub supported_codec_payload_types: Vec<u8>,
    pub trace_event_types: String,
    pub r#type: ConsumerType,
    pub consumable_rtp_encodings: Vec<ConsumableRtpEncoding>,
    pub rtp_stream: RtpStream,
    /// Essentially `Option<u8>` or `Option<-1>`
    pub preferred_spatial_layer: Option<i16>,
    /// Essentially `Option<u8>` or `Option<-1>`
    pub target_spatial_layer: Option<i16>,
    /// Essentially `Option<u8>` or `Option<-1>`
    pub current_spatial_layer: Option<i16>,
    /// Essentially `Option<u8>` or `Option<-1>`
    pub preferred_temporal_layer: Option<i16>,
    /// Essentially `Option<u8>` or `Option<-1>`
    pub target_temporal_layer: Option<i16>,
    /// Essentially `Option<u8>` or `Option<-1>`
    pub current_temporal_layer: Option<i16>,
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum ConsumerType {
    Simple,
    Simulcast,
    SVC,
    Pipe,
}

impl From<ProducerType> for ConsumerType {
    fn from(producer_type: ProducerType) -> Self {
        match producer_type {
            ProducerType::Simple => ConsumerType::Simple,
            ProducerType::Simulcast => ConsumerType::Simulcast,
            ProducerType::SVC => ConsumerType::SVC,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
pub struct ConsumerStat {
    // Common to all RtpStreams.
    // `type` field is present in worker, but ignored here
    pub timestamp: u64,
    pub ssrc: u32,
    pub rtx_ssrc: Option<u32>,
    pub kind: MediaKind,
    pub mime_type: MimeType,
    pub packets_lost: u32,
    pub fraction_lost: u8,
    pub packets_discarded: usize,
    pub packets_retransmitted: usize,
    pub packets_repaired: usize,
    pub nack_count: usize,
    pub nack_packet_count: usize,
    pub pli_count: usize,
    pub fir_count: usize,
    pub score: u8,
    pub packet_count: usize,
    pub byte_count: usize,
    pub bitrate: u32,
    pub round_trip_time: Option<u32>,
}

#[allow(clippy::large_enum_variant)]
#[derive(Debug, Deserialize, Serialize)]
#[serde(untagged)]
pub enum ConsumerStats {
    JustConsumer((ConsumerStat,)),
    WithProducer((ConsumerStat, ProducerStat)),
}

/// 'trace' event data.
#[derive(Debug, Clone, Deserialize, Serialize)]
#[serde(tag = "type", rename_all = "lowercase")]
pub enum ConsumerTraceEventData {
    RTP {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: EventDirection,
        // TODO: Clarify value structure
        /// Per type information.
        info: Value,
    },
    KeyFrame {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: EventDirection,
        // TODO: Clarify value structure
        /// Per type information.
        info: Value,
    },
    NACK {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: EventDirection,
        // TODO: Clarify value structure
        /// Per type information.
        info: Value,
    },
    PLI {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: EventDirection,
        // TODO: Clarify value structure
        /// Per type information.
        info: Value,
    },
    FIR {
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
pub enum ConsumerTraceEventType {
    RTP,
    KeyFrame,
    NACK,
    PLI,
    FIR,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    ProducerClose,
    ProducerPause,
    ProducerResume,
    Score(ConsumerScore),
    LayersChange(ConsumerLayers),
    Trace(ConsumerTraceEventData),
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum PayloadNotification {
    Rtp,
}

#[derive(Default)]
struct Handlers {
    rtp: Bag<'static, dyn Fn(&Bytes) + Send>,
    pause: Bag<'static, dyn Fn() + Send>,
    resume: Bag<'static, dyn Fn() + Send>,
    producer_pause: Bag<'static, dyn Fn() + Send>,
    producer_resume: Bag<'static, dyn Fn() + Send>,
    score: Bag<'static, dyn Fn(&ConsumerScore) + Send>,
    layers_change: Bag<'static, dyn Fn(&ConsumerLayers) + Send>,
    trace: Bag<'static, dyn Fn(&ConsumerTraceEventData) + Send>,
    producer_close: Bag<'static, dyn FnOnce() + Send>,
    transport_close: Bag<'static, dyn FnOnce() + Send>,
    close: Bag<'static, dyn FnOnce() + Send>,
}

struct Inner {
    id: ConsumerId,
    producer_id: ProducerId,
    kind: MediaKind,
    r#type: ConsumerType,
    rtp_parameters: RtpParameters,
    paused: Arc<SyncMutex<bool>>,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    producer_paused: Arc<SyncMutex<bool>>,
    priority: SyncMutex<u8>,
    score: Arc<SyncMutex<ConsumerScore>>,
    preferred_layers: SyncMutex<Option<ConsumerLayers>>,
    current_layers: Arc<SyncMutex<Option<ConsumerLayers>>>,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Option<Box<dyn Transport>>,
    // Drop subscription to consumer-specific notifications when consumer itself is dropped
    _subscription_handlers: Vec<SubscriptionHandler>,
    _on_transport_close_handler: SyncMutex<HandlerId<'static>>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.handlers.close.call_once_simple();

        {
            let channel = self.channel.clone();
            let request = ConsumerCloseRequest {
                internal: ConsumerInternal {
                    router_id: self.transport.as_ref().unwrap().router_id(),
                    transport_id: self.transport.as_ref().unwrap().id(),
                    consumer_id: self.id,
                    producer_id: self.producer_id,
                },
            };
            let transport = self.transport.take();
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("consumer closing failed on drop: {}", error);
                    }

                    drop(transport);
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct Consumer {
    inner: Arc<Inner>,
}

impl Consumer {
    #[allow(clippy::too_many_arguments)]
    pub(super) async fn new<Dump, Stat, Transport>(
        id: ConsumerId,
        producer_id: ProducerId,
        kind: MediaKind,
        r#type: ConsumerType,
        rtp_parameters: RtpParameters,
        paused: bool,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: PayloadChannel,
        producer_paused: bool,
        score: ConsumerScore,
        preferred_layers: Option<ConsumerLayers>,
        app_data: AppData,
        transport: Transport,
    ) -> Self
    where
        Dump: Debug + DeserializeOwned + 'static,
        Stat: Debug + DeserializeOwned + 'static,
        Transport: TransportGeneric<Dump, Stat> + 'static,
    {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let score = Arc::new(SyncMutex::new(score));
        #[allow(clippy::mutex_atomic)]
        let paused = Arc::new(SyncMutex::new(paused));
        #[allow(clippy::mutex_atomic)]
        let producer_paused = Arc::new(SyncMutex::new(producer_paused));
        let current_layers = Arc::<SyncMutex<Option<ConsumerLayers>>>::default();

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);
            let paused = Arc::clone(&paused);
            let producer_paused = Arc::clone(&producer_paused);
            let score = Arc::clone(&score);
            let current_layers = Arc::clone(&current_layers);

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    match serde_json::from_value::<Notification>(notification) {
                        Ok(notification) => match notification {
                            Notification::ProducerClose => {
                                handlers.producer_close.call_once_simple();
                                handlers.close.call_once_simple();
                            }
                            Notification::ProducerPause => {
                                let mut producer_paused = producer_paused.lock();
                                let was_paused = *paused.lock() || *producer_paused;
                                *producer_paused = true;

                                handlers.producer_pause.call_simple();

                                if !was_paused {
                                    handlers.pause.call_simple();
                                }
                            }
                            Notification::ProducerResume => {
                                let mut producer_paused = producer_paused.lock();
                                let paused = *paused.lock();
                                let was_paused = paused || *producer_paused;
                                *producer_paused = false;

                                handlers.producer_resume.call_simple();

                                if was_paused && !paused {
                                    handlers.resume.call_simple();
                                }
                            }
                            Notification::Score(consumer_score) => {
                                *score.lock() = consumer_score.clone();
                                handlers.score.call(|callback| {
                                    callback(&consumer_score);
                                });
                            }
                            Notification::LayersChange(consumer_layers) => {
                                *current_layers.lock() = Some(consumer_layers);
                                handlers.layers_change.call(|callback| {
                                    callback(&consumer_layers);
                                });
                            }
                            Notification::Trace(trace_event_data) => {
                                handlers.trace.call(|callback| {
                                    callback(&trace_event_data);
                                });
                            }
                        },
                        Err(error) => {
                            error!("Failed to parse notification: {}", error);
                        }
                    }
                })
                .await
        };

        let payload_subscription_handler = {
            let handlers = Arc::clone(&handlers);

            payload_channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    let NotificationMessage { message, payload } = notification;
                    match serde_json::from_value::<PayloadNotification>(message) {
                        Ok(notification) => match notification {
                            PayloadNotification::Rtp => {
                                handlers.rtp.call(|callback| {
                                    callback(&payload);
                                });
                            }
                        },
                        Err(error) => {
                            error!("Failed to parse payload notification: {}", error);
                        }
                    }
                })
                .await
        };

        let on_transport_close_handler = transport.on_close({
            let handlers = Arc::clone(&handlers);

            move || {
                handlers.transport_close.call_once_simple();
                handlers.close.call_once_simple();
            }
        });
        let inner = Arc::new(Inner {
            id,
            producer_id,
            kind,
            r#type,
            rtp_parameters,
            paused,
            producer_paused,
            priority: SyncMutex::new(1u8),
            score,
            preferred_layers: SyncMutex::new(preferred_layers),
            current_layers,
            executor,
            channel,
            handlers,
            app_data,
            transport: Some(Box::new(transport)),
            _subscription_handlers: vec![subscription_handler, payload_subscription_handler],
            _on_transport_close_handler: SyncMutex::new(on_transport_close_handler),
        });

        Self { inner }
    }

    /// Consumer id.
    pub fn id(&self) -> ConsumerId {
        self.inner.id
    }

    /// Associated Producer id.
    pub fn producer_id(&self) -> ProducerId {
        self.inner.producer_id
    }

    /// Media kind.
    pub fn kind(&self) -> MediaKind {
        self.inner.kind
    }

    /// RTP parameters.
    pub fn rtp_parameters(&self) -> &RtpParameters {
        &self.inner.rtp_parameters
    }

    /// Consumer type.
    pub fn r#type(&self) -> ConsumerType {
        self.inner.r#type
    }

    /// Whether the Consumer is paused.
    pub fn paused(&self) -> bool {
        *self.inner.paused.lock()
    }

    /// Whether the associate Producer is paused.
    pub fn producer_paused(&self) -> bool {
        *self.inner.producer_paused.lock()
    }

    /// Current priority.
    pub fn priority(&self) -> u8 {
        *self.inner.priority.lock()
    }

    /// Consumer score.
    pub fn score(&self) -> ConsumerScore {
        self.inner.score.lock().clone()
    }

    /// Preferred video layers.
    pub fn preferred_layers(&self) -> Option<ConsumerLayers> {
        *self.inner.preferred_layers.lock()
    }

    /// Current video layers.
    pub fn current_layers(&self) -> Option<ConsumerLayers> {
        *self.inner.current_layers.lock()
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    /// Dump Consumer.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<ConsumerDump, RequestError> {
        debug!("dump()");

        self.inner
            .channel
            .request(ConsumerDumpRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Get Consumer stats.
    pub async fn get_stats(&self) -> Result<ConsumerStats, RequestError> {
        debug!("get_stats()");

        self.inner
            .channel
            .request(ConsumerGetStatsRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Pause the Consumer.
    pub async fn pause(&self) -> Result<(), RequestError> {
        debug!("pause()");

        self.inner
            .channel
            .request(ConsumerPauseRequest {
                internal: self.get_internal(),
            })
            .await?;

        let mut paused = self.inner.paused.lock();
        let was_paused = *paused || *self.inner.producer_paused.lock();
        *paused = true;

        if !was_paused {
            self.inner.handlers.pause.call_simple();
        }

        Ok(())
    }

    /// Resume the Consumer.
    pub async fn resume(&self) -> Result<(), RequestError> {
        debug!("resume()");

        self.inner
            .channel
            .request(ConsumerResumeRequest {
                internal: self.get_internal(),
            })
            .await?;

        let mut paused = self.inner.paused.lock();
        let was_paused = *paused || *self.inner.producer_paused.lock();
        *paused = false;

        if was_paused {
            self.inner.handlers.resume.call_simple();
        }

        Ok(())
    }

    /// Set preferred video layers.
    pub async fn set_preferred_layers(
        &self,
        consumer_layers: ConsumerLayers,
    ) -> Result<(), RequestError> {
        debug!("set_preferred_layers()");

        let consumer_layers = self
            .inner
            .channel
            .request(ConsumerSetPreferredLayersRequest {
                internal: self.get_internal(),
                data: consumer_layers,
            })
            .await?;

        *self.inner.preferred_layers.lock() = consumer_layers;

        Ok(())
    }

    /// Set priority.
    pub async fn set_priority(&self, priority: u8) -> Result<(), RequestError> {
        debug!("set_preferred_layers()");

        let result = self
            .inner
            .channel
            .request(ConsumerSetPriorityRequest {
                internal: self.get_internal(),
                data: ConsumerSetPriorityData { priority },
            })
            .await?;

        *self.inner.priority.lock() = result.priority;

        Ok(())
    }

    /// Unset priority.
    pub async fn unset_priority(&self) -> Result<(), RequestError> {
        debug!("unset_priority()");

        let priority = 1;

        let result = self
            .inner
            .channel
            .request(ConsumerSetPriorityRequest {
                internal: self.get_internal(),
                data: ConsumerSetPriorityData { priority },
            })
            .await?;

        *self.inner.priority.lock() = result.priority;

        Ok(())
    }

    /// Request a key frame to the Producer.
    pub async fn request_key_frame(&self) -> Result<(), RequestError> {
        debug!("request_key_frame()");

        self.inner
            .channel
            .request(ConsumerRequestKeyFrameRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Enable 'trace' event.
    pub async fn enable_trace_event(
        &self,
        types: Vec<ConsumerTraceEventType>,
    ) -> Result<(), RequestError> {
        debug!("enable_trace_event()");

        self.inner
            .channel
            .request(ConsumerEnableTraceEventRequest {
                internal: self.get_internal(),
                data: ConsumerEnableTraceEventData { types },
            })
            .await
    }

    pub fn on_rtp<F: Fn(&Bytes) + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.rtp.add(Box::new(callback))
    }

    pub fn on_pause<F: Fn() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.pause.add(Box::new(callback))
    }

    pub fn on_resume<F: Fn() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.resume.add(Box::new(callback))
    }

    pub fn on_producer_pause<F: Fn() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.producer_pause.add(Box::new(callback))
    }

    pub fn on_producer_resume<F: Fn() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.producer_resume.add(Box::new(callback))
    }

    pub fn on_score<F: Fn(&ConsumerScore) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.score.add(Box::new(callback))
    }

    pub fn on_layers_change<F: Fn(&ConsumerLayers) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.layers_change.add(Box::new(callback))
    }

    pub fn on_trace<F: Fn(&ConsumerTraceEventData) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.trace.add(Box::new(callback))
    }

    pub fn on_producer_close<F: FnOnce() + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.producer_close.add(Box::new(callback))
    }

    pub fn on_transport_close<F: FnOnce() + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.transport_close.add(Box::new(callback))
    }

    pub fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.close.add(Box::new(callback))
    }

    fn get_internal(&self) -> ConsumerInternal {
        ConsumerInternal {
            router_id: self.inner.transport.as_ref().unwrap().router_id(),
            transport_id: self.inner.transport.as_ref().unwrap().id(),
            consumer_id: self.inner.id,
            producer_id: self.inner.producer_id,
        }
    }
}
