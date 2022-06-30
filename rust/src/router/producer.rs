#[cfg(test)]
mod tests;

use crate::consumer::RtpStreamParams;
use crate::data_structures::{AppData, RtpPacketTraceInfo, SsrcTraceInfo, TraceEventDirection};
use crate::messages::{
    ProducerCloseRequest, ProducerDumpRequest, ProducerEnableTraceEventData,
    ProducerEnableTraceEventRequest, ProducerGetStatsRequest, ProducerInternal,
    ProducerPauseRequest, ProducerResumeRequest, ProducerSendNotification,
};
pub use crate::ortc::RtpMapping;
use crate::rtp_parameters::{MediaKind, MimeType, RtpParameters};
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::{
    Channel, NotificationError, PayloadChannel, RequestError, SubscriptionHandler,
};
use async_executor::Executor;
use event_listener_primitives::{Bag, BagOnce, HandlerId};
use hash_hasher::HashedMap;
use log::{debug, error};
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use serde_repr::{Deserialize_repr, Serialize_repr};
use std::fmt;
use std::fmt::Debug;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Weak};

uuid_based_wrapper_type!(
    /// [`Producer`] identifier.
    ProducerId
);

/// [`Producer`] options.
///
/// # Notes on usage
/// Check the
/// [RTP Parameters and Capabilities](https://mediasoup.org/documentation/v3/mediasoup/rtp-parameters-and-capabilities/)
/// section for more details (TypeScript-oriented, but concepts apply here as well).
#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct ProducerOptions {
    /// Producer id (just for
    /// [`Router::pipe_producer_to_router`](crate::router::Router::pipe_producer_to_router) method).
    pub(super) id: Option<ProducerId>,
    /// Media kind.
    pub kind: MediaKind,
    /// RTP parameters defining what the endpoint is sending.
    pub rtp_parameters: RtpParameters,
    /// Whether the producer must start in paused mode. Default false.
    pub paused: bool,
    /// Just for video. Time (in ms) before asking the sender for a new key frame after having asked
    /// a previous one. If 0 there is no delay.
    pub key_frame_request_delay: u32,
    /// Custom application data.
    pub app_data: AppData,
}

impl ProducerOptions {
    /// Create producer options that will be used with Pipe transport
    #[must_use]
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

    /// Create producer options that will be used with non-Pipe transport
    #[must_use]
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
    pub bitrate_by_layer: Option<HashedMap<String, u32>>,
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

/// Producer type.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum ProducerType {
    /// A single RTP stream is received with no spatial/temporal layers.
    Simple,
    /// Two or more RTP streams are received, each of them with one or more temporal layers.
    Simulcast,
    /// A single RTP stream is received with spatial/temporal layers.
    Svc,
}

/// Score of the RTP stream in the producer representing its transmission quality.
#[derive(Debug, Clone, Eq, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ProducerScore {
    /// Index of the RTP stream in the [`RtpParameters::encodings`] array of the producer.
    pub encoding_idx: u32,
    /// RTP stream SSRC.
    pub ssrc: u32,
    /// RTP stream RID value.
    pub rid: Option<String>,
    /// RTP stream score (from 0 to 10) representing the transmission quality.
    pub score: u8,
}

/// Rotation angle
#[derive(Debug, Copy, Clone, Eq, PartialEq, Deserialize_repr, Serialize_repr)]
#[repr(u16)]
pub enum Rotation {
    /// 0
    None = 0,
    /// 90 (clockwise)
    Clockwise = 90,
    /// 180
    Rotate180 = 180,
    /// 270 (90 counter-clockwise)
    CounterClockwise = 270,
}

/// As documented in
/// [WebRTC Video Processing and Codec Requirements](https://tools.ietf.org/html/rfc7742#section-4).
#[derive(Debug, Copy, Clone, Deserialize, Serialize)]
pub struct ProducerVideoOrientation {
    /// Whether the source is a video camera.
    pub camera: bool,
    /// Whether the video source is flipped.
    pub flip: bool,
    /// Rotation degrees.
    pub rotation: Rotation,
}

/// RTC statistics of the producer.
#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
#[allow(missing_docs)]
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
    pub round_trip_time: Option<f32>,
    pub rtx_packets_discarded: Option<u32>,
    // RtpStreamRecv specific.
    pub jitter: u32,
    pub bitrate_by_layer: Option<HashedMap<String, u32>>,
}

/// 'trace' event data.
#[derive(Debug, Clone, Deserialize, Serialize)]
#[serde(tag = "type", rename_all = "lowercase")]
pub enum ProducerTraceEventData {
    /// RTP packet.
    Rtp {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: TraceEventDirection,
        /// RTP packet info.
        info: RtpPacketTraceInfo,
    },
    /// RTP video keyframe packet.
    KeyFrame {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: TraceEventDirection,
        /// RTP packet info.
        info: RtpPacketTraceInfo,
    },
    /// RTCP NACK packet.
    Nack {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: TraceEventDirection,
    },
    /// RTCP PLI packet.
    Pli {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: TraceEventDirection,
        /// SSRC info.
        info: SsrcTraceInfo,
    },
    /// RTCP FIR packet.
    Fir {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: TraceEventDirection,
        /// SSRC info.
        info: SsrcTraceInfo,
    },
}

/// Types of consumer trace events.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum ProducerTraceEventType {
    /// RTP packet.
    Rtp,
    /// RTP video keyframe packet.
    KeyFrame,
    /// RTCP NACK packet.
    Nack,
    /// RTCP PLI packet.
    Pli,
    /// RTCP FIR packet.
    Fir,
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
    score: Bag<Arc<dyn Fn(&[ProducerScore]) + Send + Sync>>,
    video_orientation_change: Bag<Arc<dyn Fn(ProducerVideoOrientation) + Send + Sync>>,
    pause: Bag<Arc<dyn Fn() + Send + Sync>>,
    resume: Bag<Arc<dyn Fn() + Send + Sync>>,
    trace: Bag<Arc<dyn Fn(&ProducerTraceEventData) + Send + Sync>, ProducerTraceEventData>,
    transport_close: BagOnce<Box<dyn FnOnce() + Send>>,
    close: BagOnce<Box<dyn FnOnce() + Send>>,
}

struct Inner {
    id: ProducerId,
    kind: MediaKind,
    r#type: ProducerType,
    rtp_parameters: RtpParameters,
    consumable_rtp_parameters: RtpParameters,
    direct: bool,
    paused: AtomicBool,
    score: Arc<Mutex<Vec<ProducerScore>>>,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Arc<dyn Transport>,
    closed: AtomicBool,
    // Drop subscription to producer-specific notifications when producer itself is dropped
    _subscription_handler: Mutex<Option<SubscriptionHandler>>,
    _on_transport_close_handler: Mutex<HandlerId>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.close(true);
    }
}

impl Inner {
    fn close(&self, close_request: bool) {
        if !self.closed.swap(true, Ordering::SeqCst) {
            debug!("close()");

            self.handlers.close.call_simple();

            if close_request {
                let channel = self.channel.clone();
                let request = ProducerCloseRequest {
                    internal: ProducerInternal {
                        router_id: self.transport.router().id(),
                        transport_id: self.transport.id(),
                        producer_id: self.id,
                    },
                };

                self.executor
                    .spawn(async move {
                        if let Err(error) = channel.request(request).await {
                            error!("producer closing failed on drop: {}", error);
                        }
                    })
                    .detach();
            }
        }
    }
}

/// Producer created on transport other than
/// [`DirectTransport`](crate::direct_transport::DirectTransport).
#[derive(Clone)]
#[must_use = "Producer will be closed on drop, make sure to keep it around for as long as needed"]
pub struct RegularProducer {
    inner: Arc<Inner>,
}

impl fmt::Debug for RegularProducer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RegularProducer")
            .field("id", &self.inner.id)
            .field("kind", &self.inner.kind)
            .field("type", &self.inner.r#type)
            .field("rtp_parameters", &self.inner.rtp_parameters)
            .field(
                "consumable_rtp_parameters",
                &self.inner.consumable_rtp_parameters,
            )
            .field("paused", &self.inner.paused)
            .field("score", &self.inner.score)
            .field("transport", &self.inner.transport)
            .field("closed", &self.inner.closed)
            .finish()
    }
}

impl From<RegularProducer> for Producer {
    fn from(producer: RegularProducer) -> Self {
        Producer::Regular(producer)
    }
}

/// Producer created on [`DirectTransport`](crate::direct_transport::DirectTransport).
#[derive(Clone)]
#[must_use = "Producer will be closed on drop, make sure to keep it around for as long as needed"]
pub struct DirectProducer {
    inner: Arc<Inner>,
}

impl fmt::Debug for DirectProducer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DirectProducer")
            .field("id", &self.inner.id)
            .field("kind", &self.inner.kind)
            .field("type", &self.inner.r#type)
            .field("rtp_parameters", &self.inner.rtp_parameters)
            .field(
                "consumable_rtp_parameters",
                &self.inner.consumable_rtp_parameters,
            )
            .field("paused", &self.inner.paused)
            .field("score", &self.inner.score)
            .field("transport", &self.inner.transport)
            .field("closed", &self.inner.closed)
            .finish()
    }
}

impl From<DirectProducer> for Producer {
    fn from(producer: DirectProducer) -> Self {
        Producer::Direct(producer)
    }
}

/// A producer represents an audio or video source being injected into a mediasoup router. It's
/// created on top of a transport that defines how the media packets are carried.
#[derive(Clone)]
#[non_exhaustive]
#[must_use = "Producer will be closed on drop, make sure to keep it around for as long as needed"]
pub enum Producer {
    /// Producer created on transport other than
    /// [`DirectTransport`](crate::direct_transport::DirectTransport).
    Regular(RegularProducer),
    /// Producer created on [`DirectTransport`](crate::direct_transport::DirectTransport).
    Direct(DirectProducer),
}

impl fmt::Debug for Producer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self {
            Producer::Regular(producer) => f.debug_tuple("Regular").field(&producer).finish(),
            Producer::Direct(producer) => f.debug_tuple("Direct").field(&producer).finish(),
        }
    }
}

impl Producer {
    #[allow(clippy::too_many_arguments)]
    pub(super) async fn new(
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
        transport: Arc<dyn Transport>,
        direct: bool,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let score = Arc::<Mutex<Vec<ProducerScore>>>::default();

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);
            let score = Arc::clone(&score);

            channel.subscribe_to_notifications(id.into(), move |notification| {
                match serde_json::from_slice::<Notification>(notification) {
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
                            handlers.trace.call_simple(&trace_event_data);
                        }
                    },
                    Err(error) => {
                        error!("Failed to parse notification: {}", error);
                    }
                }
            })
        };

        let inner_weak = Arc::<Mutex<Option<Weak<Inner>>>>::default();
        let on_transport_close_handler = transport.on_close({
            let inner_weak = Arc::clone(&inner_weak);

            Box::new(move || {
                let maybe_inner = inner_weak.lock().as_ref().and_then(Weak::upgrade);
                if let Some(inner) = maybe_inner {
                    inner.handlers.transport_close.call_simple();
                    inner.close(false);
                }
            })
        });
        let inner = Arc::new(Inner {
            id,
            kind,
            r#type,
            rtp_parameters,
            consumable_rtp_parameters,
            direct,
            paused: AtomicBool::new(paused),
            score,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport,
            closed: AtomicBool::new(false),
            _subscription_handler: Mutex::new(subscription_handler),
            _on_transport_close_handler: Mutex::new(on_transport_close_handler),
        });

        inner_weak.lock().replace(Arc::downgrade(&inner));

        if direct {
            Self::Direct(DirectProducer { inner })
        } else {
            Self::Regular(RegularProducer { inner })
        }
    }

    /// Producer identifier.
    #[must_use]
    pub fn id(&self) -> ProducerId {
        self.inner().id
    }

    /// Transport to which producer belongs.
    pub fn transport(&self) -> &Arc<dyn Transport> {
        &self.inner().transport
    }

    /// Media kind.
    #[must_use]
    pub fn kind(&self) -> MediaKind {
        self.inner().kind
    }

    /// Producer RTP parameters.
    /// # Notes on usage
    /// Check the
    /// [RTP Parameters and Capabilities](https://mediasoup.org/documentation/v3/mediasoup/rtp-parameters-and-capabilities/)
    /// section for more details (TypeScript-oriented, but concepts apply here as well).
    #[must_use]
    pub fn rtp_parameters(&self) -> &RtpParameters {
        &self.inner().rtp_parameters
    }

    /// Producer type.
    #[must_use]
    pub fn r#type(&self) -> ProducerType {
        self.inner().r#type
    }

    /// Whether the Producer is paused.
    #[must_use]
    pub fn paused(&self) -> bool {
        self.inner().paused.load(Ordering::SeqCst)
    }

    /// The score of each RTP stream being received, representing their transmission quality.
    #[must_use]
    pub fn score(&self) -> Vec<ProducerScore> {
        self.inner().score.lock().clone()
    }

    /// Custom application data.
    #[must_use]
    pub fn app_data(&self) -> &AppData {
        &self.inner().app_data
    }

    /// Whether the producer is closed.
    #[must_use]
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

    /// Returns current RTC statistics of the producer.
    ///
    /// Check the [RTC Statistics](https://mediasoup.org/documentation/v3/mediasoup/rtc-statistics/)
    /// section for more details (TypeScript-oriented, but concepts apply here as well).
    pub async fn get_stats(&self) -> Result<Vec<ProducerStat>, RequestError> {
        debug!("get_stats()");

        self.inner()
            .channel
            .request(ProducerGetStatsRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Pauses the producer (no RTP is sent to its associated consumers).  Calls
    /// [`Consumer::on_producer_pause`](crate::consumer::Consumer::on_producer_pause) callback on
    /// all its associated consumers.
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

    /// Resumes the producer (no RTP is sent to its associated consumers). Calls
    /// [`Consumer::on_producer_resume`](crate::consumer::Consumer::on_producer_resume) callback on
    /// all its associated consumers.
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

    /// Instructs the procuer to emit "trace" events. For monitoring purposes. Use with caution.
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

    /// Callback is called when the producer score changes.
    pub fn on_score<F: Fn(&[ProducerScore]) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner().handlers.score.add(Arc::new(callback))
    }

    /// Callback is called when the video orientation changes. This is just possible if the
    /// `urn:3gpp:video-orientation` RTP extension has been negotiated in the producer RTP
    /// parameters.
    pub fn on_video_orientation_change<F: Fn(ProducerVideoOrientation) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner()
            .handlers
            .video_orientation_change
            .add(Arc::new(callback))
    }

    /// Callback is called when the producer is paused.
    pub fn on_pause<F: Fn() + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner().handlers.pause.add(Arc::new(callback))
    }

    /// Callback is called when the producer is resumed.
    pub fn on_resume<F: Fn() + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner().handlers.resume.add(Arc::new(callback))
    }

    /// See [`Producer::enable_trace_event`] method.
    pub fn on_trace<F: Fn(&ProducerTraceEventData) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner().handlers.trace.add(Arc::new(callback))
    }

    /// Callback is called when the transport this producer belongs to is closed for whatever
    /// reason. The producer itself is also closed. A `on_producer_close` callback is called on all
    /// its associated consumers.
    pub fn on_transport_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner()
            .handlers
            .transport_close
            .add(Box::new(callback))
    }

    /// Callback is called when the producer is closed for whatever reason.
    ///
    /// NOTE: Callback will be called in place if producer is already closed.
    pub fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        let handler_id = self.inner().handlers.close.add(Box::new(callback));
        if self.inner().closed.load(Ordering::Relaxed) {
            self.inner().handlers.close.call_simple();
        }
        handler_id
    }

    /// Consumable RTP parameters.
    // This is used in tests, otherwise would have been `pub(super)`
    #[doc(hidden)]
    #[must_use]
    pub fn consumable_rtp_parameters(&self) -> &RtpParameters {
        &self.inner().consumable_rtp_parameters
    }

    pub(super) fn close(&self) {
        self.inner().close(true);
    }

    /// Downgrade `Producer` to [`WeakProducer`] instance.
    #[must_use]
    pub fn downgrade(&self) -> WeakProducer {
        WeakProducer {
            inner: Arc::downgrade(self.inner()),
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
            router_id: self.inner().transport.router().id(),
            transport_id: self.inner().transport.id(),
            producer_id: self.inner().id,
        }
    }
}

impl DirectProducer {
    /// Sends a RTP packet from the Rust process.
    pub fn send(&self, rtp_packet: Vec<u8>) -> Result<(), NotificationError> {
        self.inner.payload_channel.notify(
            ProducerSendNotification {
                internal: ProducerInternal {
                    router_id: self.inner.transport.router().id(),
                    transport_id: self.inner.transport.id(),
                    producer_id: self.inner.id,
                },
            },
            rtp_packet,
        )
    }
}

/// Same as [`Producer`], but will not be closed when dropped.
///
/// The idea here is that [`ProducerId`] of both original [`Producer`] and `PipedProducer` is the
/// same and lifetime of piped producer is also tied to original producer, so you may not need to
/// store `PipedProducer` at all.
///
/// Use [`PipedProducer::into_inner()`] method to get regular [`Producer`] instead and restore
/// regular behavior of [`Drop`] implementation.
pub struct PipedProducer {
    producer: Producer,
    on_drop: Option<Box<dyn FnOnce(Producer) + Send + 'static>>,
}

impl fmt::Debug for PipedProducer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("PipedProducer")
            .field("producer", &self.producer)
            .finish()
    }
}

impl Drop for PipedProducer {
    fn drop(&mut self) {
        if let Some(on_drop) = self.on_drop.take() {
            on_drop(self.producer.clone())
        }
    }
}

impl PipedProducer {
    /// * `on_drop` - Callback that takes last `Producer` instance and must do something with it to
    ///   prevent dropping and thus closing
    pub(crate) fn new<F: FnOnce(Producer) + Send + 'static>(
        producer: Producer,
        on_drop: F,
    ) -> Self {
        Self {
            producer,
            on_drop: Some(Box::new(on_drop)),
        }
    }

    /// Get inner [`Producer`] (which will close on drop in contrast to `PipedProducer`).
    pub fn into_inner(mut self) -> Producer {
        self.on_drop.take();
        self.producer.clone()
    }
}

/// [`WeakProducer`] doesn't own producer instance on mediasoup-worker and will not prevent one from
/// being destroyed once last instance of regular [`Producer`] is dropped.
///
/// [`WeakProducer`] vs [`Producer`] is similar to [`Weak`] vs [`Arc`].
#[derive(Clone)]
pub struct WeakProducer {
    inner: Weak<Inner>,
}

impl fmt::Debug for WeakProducer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("WeakProducer").finish()
    }
}

impl WeakProducer {
    /// Attempts to upgrade `WeakProducer` to [`Producer`] if last instance of one wasn't dropped
    /// yet.
    #[must_use]
    pub fn upgrade(&self) -> Option<Producer> {
        let inner = self.inner.upgrade()?;

        let producer = if inner.direct {
            Producer::Direct(DirectProducer { inner })
        } else {
            Producer::Regular(RegularProducer { inner })
        };

        Some(producer)
    }
}
