use crate::consumer::RtpStreamParams;
use crate::data_structures::{AppData, EventDirection};
use crate::messages::{
    ProducerCloseRequest, ProducerDumpRequest, ProducerEnableTraceEventData,
    ProducerEnableTraceEventRequest, ProducerGetStatsRequest, ProducerInternal,
    ProducerPauseRequest, ProducerResumeRequest, ProducerSendNotification,
};
use crate::ortc::RtpMapping;
use crate::rtp_parameters::{MediaKind, MimeType, RtpParameters};
use crate::transport::{Transport, TransportGeneric};
use crate::uuid_based_wrapper_type;
use crate::worker::{
    Channel, NotificationError, PayloadChannel, RequestError, SubscriptionHandler,
};
use async_executor::Executor;
use bytes::Bytes;
use event_listener_primitives::{Bag, BagOnce, HandlerId};
use log::*;
use parking_lot::Mutex as SyncMutex;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::fmt::Debug;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Weak};

uuid_based_wrapper_type!(ProducerId);

#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct ProducerOptions {
    /// Producer id (just for Router.pipeToRouter() method).
    /// Producer id, should most likely not be specified explicitly, specified by pipe transport
    pub(super) id: Option<ProducerId>,
    /// Media kind.
    pub kind: MediaKind,
    // TODO: Docs have distinction between RtpSendParameters and RtpReceiveParameters
    /// RTP parameters defining what the endpoint is sending.
    pub rtp_parameters: RtpParameters,
    /// Whether the producer must start in paused mode. Default false.
    pub paused: bool,
    /// Just for video. Time (in ms) before asking the sender for a new key frame
    pub key_frame_request_delay: u32,
    /// Custom application data.
    pub app_data: AppData,
}

impl ProducerOptions {
    pub fn new_pipe_transport(
        producer_id: ProducerId,
        kind: MediaKind,
        rtp_parameters: RtpParameters,
    ) -> Self {
        Self {
            id: Some(producer_id),
            kind,
            rtp_parameters,
            paused: false,
            key_frame_request_delay: 0,
            app_data: AppData::default(),
        }
    }

    pub fn new(kind: MediaKind, rtp_parameters: RtpParameters) -> Self {
        Self {
            id: None,
            kind,
            rtp_parameters,
            paused: false,
            key_frame_request_delay: 0,
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RtpStreamRecv {
    pub params: RtpStreamParams,
    pub score: u8,
    // `type` field is present in worker, but ignored here
    pub jitter: u32,
    pub packet_count: usize,
    pub byte_count: usize,
    pub bitrate: u32,
    pub bitrate_by_layer: Option<HashMap<String, u32>>,
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
#[non_exhaustive]
pub struct ProducerDump {
    pub id: ProducerId,
    pub kind: MediaKind,
    pub paused: bool,
    pub rtp_mapping: RtpMapping,
    pub rtp_parameters: RtpParameters,
    pub rtp_streams: Vec<RtpStreamRecv>,
    pub trace_event_types: String,
    pub r#type: ProducerType,
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum ProducerType {
    Simple,
    Simulcast,
    SVC,
}

#[derive(Debug, Clone, Eq, PartialEq, Deserialize, Serialize)]
pub struct ProducerScore {
    // TODO: C++ code also seems to have `encodingIdx`, shouldn't we add it here?
    /// SSRC of the RTP stream.
    pub ssrc: u32,
    /// RID of the RTP stream.
    pub rid: Option<String>,
    /// The score of the RTP stream.
    pub score: u8,
}

#[derive(Debug, Copy, Clone, Deserialize, Serialize)]
pub struct ProducerVideoOrientation {
    /// Whether the source is a video camera.
    pub camera: bool,
    /// Whether the video source is flipped.
    pub flip: bool,
    /// Rotation degrees (0, 90, 180 or 270).
    pub rotation: u16,
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
pub struct ProducerStat {
    // Common to all RtpStreams.
    // `type` field is present in worker, but ignored here
    pub timestamp: u64,
    pub ssrc: u32,
    pub rtx_ssrc: Option<u32>,
    pub rid: Option<String>,
    pub kind: String,
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
    // RtpStreamRecv specific.
    pub jitter: u32,
    pub bitrate_by_layer: HashMap<String, u32>,
}

/// 'trace' event data.
#[derive(Debug, Clone, Deserialize, Serialize)]
#[serde(tag = "type", rename_all = "lowercase")]
pub enum ProducerTraceEventData {
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
pub enum ProducerTraceEventType {
    RTP,
    KeyFrame,
    NACK,
    PLI,
    FIR,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    Score(Vec<ProducerScore>),
    VideoOrientationChange(ProducerVideoOrientation),
    Trace(ProducerTraceEventData),
}

#[derive(Default)]
struct Handlers {
    score: Bag<Box<dyn Fn(&Vec<ProducerScore>) + Send + Sync>>,
    video_orientation_change: Bag<Box<dyn Fn(ProducerVideoOrientation) + Send + Sync>>,
    pause: Bag<Box<dyn Fn() + Send + Sync>>,
    resume: Bag<Box<dyn Fn() + Send + Sync>>,
    trace: Bag<Box<dyn Fn(&ProducerTraceEventData) + Send + Sync>>,
    transport_close: BagOnce<Box<dyn FnOnce() + Send>>,
    close: BagOnce<Box<dyn FnOnce() + Send>>,
}

struct Inner {
    id: ProducerId,
    kind: MediaKind,
    r#type: ProducerType,
    rtp_parameters: RtpParameters,
    consumable_rtp_parameters: RtpParameters,
    paused: AtomicBool,
    score: Arc<SyncMutex<Vec<ProducerScore>>>,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Arc<Box<dyn Transport>>,
    closed: AtomicBool,
    // Drop subscription to producer-specific notifications when producer itself is dropped
    _subscription_handler: SubscriptionHandler,
    _on_transport_close_handler: SyncMutex<HandlerId>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.close();
    }
}

impl Inner {
    fn close(&self) {
        if !self.closed.swap(true, Ordering::SeqCst) {
            debug!("close()");

            self.handlers.close.call_simple();

            {
                let channel = self.channel.clone();
                let request = ProducerCloseRequest {
                    internal: ProducerInternal {
                        router_id: self.transport.router_id(),
                        transport_id: self.transport.id(),
                        producer_id: self.id,
                    },
                };
                let transport = Arc::clone(&self.transport);
                self.executor
                    .spawn(async move {
                        if let Err(error) = channel.request(request).await {
                            error!("producer closing failed on drop: {}", error);
                        }

                        drop(transport);
                    })
                    .detach();
            }
        }
    }
}

#[derive(Clone)]
pub struct RegularProducer {
    inner: Arc<Inner>,
}

impl From<RegularProducer> for Producer {
    fn from(producer: RegularProducer) -> Self {
        Producer::Regular(producer)
    }
}

#[derive(Clone)]
pub struct DirectProducer {
    inner: Arc<Inner>,
}

impl From<DirectProducer> for Producer {
    fn from(producer: DirectProducer) -> Self {
        Producer::Direct(producer)
    }
}

#[derive(Clone)]
#[non_exhaustive]
pub enum Producer {
    Regular(RegularProducer),
    Direct(DirectProducer),
}

impl Producer {
    #[allow(clippy::too_many_arguments)]
    pub(super) async fn new<Dump, Stat, Transport>(
        id: ProducerId,
        kind: MediaKind,
        r#type: ProducerType,
        rtp_parameters: RtpParameters,
        consumable_rtp_parameters: RtpParameters,
        paused: bool,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: PayloadChannel,
        app_data: AppData,
        transport: Transport,
        direct: bool,
    ) -> Self
    where
        Dump: Debug + DeserializeOwned + 'static,
        Stat: Debug + DeserializeOwned + 'static,
        Transport: TransportGeneric<Dump, Stat> + 'static,
    {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let score = Arc::<SyncMutex<Vec<ProducerScore>>>::default();

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);
            let score = Arc::clone(&score);

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    match serde_json::from_value::<Notification>(notification) {
                        Ok(notification) => match notification {
                            Notification::Score(scores) => {
                                *score.lock() = scores.clone();
                                handlers.score.call(|callback| {
                                    callback(&scores);
                                });
                            }
                            Notification::VideoOrientationChange(video_orientation) => {
                                handlers.video_orientation_change.call(|callback| {
                                    callback(video_orientation);
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

        let inner_weak = Arc::<SyncMutex<Option<Weak<Inner>>>>::default();
        let on_transport_close_handler = transport.on_close({
            let inner_weak = Arc::clone(&inner_weak);

            move || {
                if let Some(inner) = inner_weak
                    .lock()
                    .as_ref()
                    .and_then(|weak_inner| weak_inner.upgrade())
                {
                    inner.handlers.transport_close.call_simple();
                    inner.close();
                }
            }
        });
        let inner = Arc::new(Inner {
            id,
            kind,
            r#type,
            rtp_parameters,
            consumable_rtp_parameters,
            paused: AtomicBool::new(paused),
            score,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport: Arc::new(Box::new(transport)),
            closed: AtomicBool::new(false),
            _subscription_handler: subscription_handler,
            _on_transport_close_handler: SyncMutex::new(on_transport_close_handler),
        });

        inner_weak.lock().replace(Arc::downgrade(&inner));

        if direct {
            Self::Direct(DirectProducer { inner })
        } else {
            Self::Regular(RegularProducer { inner })
        }
    }

    /// Producer id.
    pub fn id(&self) -> ProducerId {
        self.inner().id
    }

    /// Media kind.
    pub fn kind(&self) -> MediaKind {
        self.inner().kind
    }

    /// Media kind.
    pub fn rtp_parameters(&self) -> &RtpParameters {
        &self.inner().rtp_parameters
    }

    /// Producer type.
    pub fn r#type(&self) -> ProducerType {
        self.inner().r#type
    }

    /// Whether the Producer is paused.
    pub fn paused(&self) -> bool {
        self.inner().paused.load(Ordering::SeqCst)
    }

    /// Producer score list.
    pub fn score(&self) -> Vec<ProducerScore> {
        self.inner().score.lock().clone()
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner().app_data
    }

    pub fn closed(&self) -> bool {
        self.inner().closed.load(Ordering::SeqCst)
    }

    /// Dump Producer.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<ProducerDump, RequestError> {
        debug!("dump()");

        self.inner()
            .channel
            .request(ProducerDumpRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Get Producer stats.
    pub async fn get_stats(&self) -> Result<Vec<ProducerStat>, RequestError> {
        debug!("get_stats()");

        self.inner()
            .channel
            .request(ProducerGetStatsRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Pause the Producer.
    pub async fn pause(&self) -> Result<(), RequestError> {
        debug!("pause()");

        self.inner()
            .channel
            .request(ProducerPauseRequest {
                internal: self.get_internal(),
            })
            .await?;

        let was_paused = self.inner().paused.swap(true, Ordering::SeqCst);

        if !was_paused {
            self.inner().handlers.pause.call_simple();
        }

        Ok(())
    }

    /// Resume the Producer.
    pub async fn resume(&self) -> Result<(), RequestError> {
        debug!("resume()");

        self.inner()
            .channel
            .request(ProducerResumeRequest {
                internal: self.get_internal(),
            })
            .await?;

        let was_paused = self.inner().paused.swap(false, Ordering::SeqCst);

        if was_paused {
            self.inner().handlers.resume.call_simple();
        }

        Ok(())
    }

    /// Enable 'trace' event.
    pub async fn enable_trace_event(
        &self,
        types: Vec<ProducerTraceEventType>,
    ) -> Result<(), RequestError> {
        debug!("enable_trace_event()");

        self.inner()
            .channel
            .request(ProducerEnableTraceEventRequest {
                internal: self.get_internal(),
                data: ProducerEnableTraceEventData { types },
            })
            .await
    }

    pub fn on_score<F: Fn(&Vec<ProducerScore>) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner().handlers.score.add(Box::new(callback))
    }

    pub fn on_video_orientation_change<F: Fn(ProducerVideoOrientation) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner()
            .handlers
            .video_orientation_change
            .add(Box::new(callback))
    }

    pub fn on_pause<F: Fn() + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner().handlers.pause.add(Box::new(callback))
    }

    pub fn on_resume<F: Fn() + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner().handlers.resume.add(Box::new(callback))
    }

    pub fn on_trace<F: Fn(&ProducerTraceEventData) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner().handlers.trace.add(Box::new(callback))
    }

    pub fn on_transport_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner()
            .handlers
            .transport_close
            .add(Box::new(callback))
    }

    pub fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner().handlers.close.add(Box::new(callback))
    }

    /// Consumable RTP parameters.
    // This is used in tests, otherwise would have been `pub(super)`
    #[doc(hidden)]
    pub fn consumable_rtp_parameters(&self) -> &RtpParameters {
        &self.inner().consumable_rtp_parameters
    }

    pub(super) fn close(&self) {
        self.inner().close();
    }

    pub(super) fn downgrade(&self) -> WeakProducer {
        WeakProducer {
            inner: Arc::downgrade(&self.inner()),
        }
    }

    fn inner(&self) -> &Arc<Inner> {
        match self {
            Producer::Regular(producer) => &producer.inner,
            Producer::Direct(producer) => &producer.inner,
        }
    }

    fn get_internal(&self) -> ProducerInternal {
        ProducerInternal {
            router_id: self.inner().transport.router_id(),
            transport_id: self.inner().transport.id(),
            producer_id: self.inner().id,
        }
    }
}

impl DirectProducer {
    /// Send RTP packet.
    pub async fn send(&self, rtp_packet: Bytes) -> Result<(), NotificationError> {
        self.inner
            .payload_channel
            .notify(
                ProducerSendNotification {
                    internal: ProducerInternal {
                        router_id: self.inner.transport.router_id(),
                        transport_id: self.inner.transport.id(),
                        producer_id: self.inner.id,
                    },
                },
                rtp_packet,
            )
            .await
    }
}

#[derive(Clone)]
pub(super) struct WeakProducer {
    inner: Weak<Inner>,
}

impl WeakProducer {
    pub(super) fn upgrade(&self) -> Option<Producer> {
        Some(Producer::Regular(RegularProducer {
            inner: self.inner.upgrade()?,
        }))
    }
}
