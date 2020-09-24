use crate::data_structures::{AppData, EventDirection, ProducerInternal};
use crate::messages::{ProducerCloseRequest, ProducerDumpRequest, ProducerGetStatsRequest};
use crate::ortc::RtpMapping;
use crate::router::RouterId;
use crate::rtp_parameters::{MediaKind, MimeType, RtpParameters};
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::{Channel, RequestError, SubscriptionHandler};
use async_executor::Executor;
use log::{debug, error};
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::HashMap;
use std::mem;
use std::sync::{Arc, Mutex};

uuid_based_wrapper_type!(ProducerId);

#[derive(Debug)]
#[non_exhaustive]
pub struct ProducerOptions {
    /// Producer id, should most likely not be specified explicitly, specified by plain transport
    #[doc(hidden)]
    pub id: Option<ProducerId>,
    pub kind: MediaKind,
    // TODO: Docs have distinction between RtpSendParameters and RtpReceiveParameters
    pub rtp_parameters: RtpParameters,
    pub paused: bool,
    pub key_frame_request_delay: u32,
    pub app_data: AppData,
}

impl ProducerOptions {
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

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct ProducerDump {
    pub id: ProducerId,
    pub kind: MediaKind,
    pub paused: bool,
    pub rtp_mapping: RtpMapping,
    pub rtp_parameters: RtpParameters,
    // TODO: Add type here
    pub rtp_streams: Vec<()>,
    // TODO: What is this field format?
    pub trace_event_types: String,
    pub r#type: ProducerType,
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum ProducerType {
    None,
    Simple,
    Simulcast,
    SVC,
    Pipe,
}

#[derive(Default)]
struct Handlers {
    score: Mutex<Vec<Box<dyn Fn(&Vec<ProducerScore>) + Send>>>,
    video_orientation_change: Mutex<Vec<Box<dyn Fn(ProducerVideoOrientation) + Send>>>,
    trace: Mutex<Vec<Box<dyn Fn(&ProducerTraceEventData) + Send>>>,
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

#[derive(Debug, Clone, Deserialize, Serialize)]
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

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub enum RtpType {
    #[serde(rename = "inbound-rtp")]
    Inbound,
    #[serde(rename = "outbound-rtp")]
    Outbound,
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ProducerStat {
    // Common to all RtpStreams.
    pub r#type: RtpType,
    pub timestamp: u32,
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

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    #[serde(rename_all = "camelCase")]
    Score(Vec<ProducerScore>),
    #[serde(rename_all = "camelCase")]
    VideoOrientationChange(ProducerVideoOrientation),
    #[serde(rename_all = "camelCase")]
    Trace(ProducerTraceEventData),
}

struct Inner {
    id: ProducerId,
    kind: MediaKind,
    r#type: ProducerType,
    rtp_parameters: RtpParameters,
    consumable_rtp_parameters: RtpParameters,
    paused: bool,
    router_id: RouterId,
    score: Arc<Mutex<Vec<ProducerScore>>>,
    executor: Arc<Executor>,
    channel: Channel,
    payload_channel: Channel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Box<dyn Transport>,
    // Drop subscription to producer-specific notifications when producer itself is dropped
    _subscription_handler: SubscriptionHandler,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        let callbacks: Vec<_> = mem::take(self.handlers.closed.lock().unwrap().as_mut());
        for callback in callbacks {
            callback();
        }

        {
            let channel = self.channel.clone();
            let request = ProducerCloseRequest {
                internal: ProducerInternal {
                    router_id: self.router_id,
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

#[derive(Clone)]
pub struct Producer {
    inner: Arc<Inner>,
}

impl Producer {
    pub(super) async fn new(
        id: ProducerId,
        kind: MediaKind,
        r#type: ProducerType,
        rtp_parameters: RtpParameters,
        consumable_rtp_parameters: RtpParameters,
        paused: bool,
        router_id: RouterId,
        executor: Arc<Executor>,
        channel: Channel,
        payload_channel: Channel,
        app_data: AppData,
        transport: Box<dyn Transport>,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let score = Arc::<Mutex<Vec<ProducerScore>>>::default();

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);
            let score = Arc::clone(&score);

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    match serde_json::from_value::<Notification>(notification) {
                        Ok(notification) => match notification {
                            Notification::Score(scores) => {
                                *score.lock().unwrap() = scores.clone();
                                for callback in handlers.score.lock().unwrap().iter() {
                                    callback(&scores);
                                }
                            }
                            Notification::VideoOrientationChange(video_orientation) => {
                                for callback in
                                    handlers.video_orientation_change.lock().unwrap().iter()
                                {
                                    callback(video_orientation);
                                }
                            }
                            Notification::Trace(trace_event_data) => {
                                for callback in handlers.trace.lock().unwrap().iter() {
                                    callback(&trace_event_data);
                                }
                            }
                        },
                        Err(error) => {
                            error!("Failed to parse notification: {}", error);
                        }
                    }
                })
                .await
                .unwrap()
        };

        let inner = Arc::new(Inner {
            id,
            kind,
            r#type,
            rtp_parameters,
            consumable_rtp_parameters,
            paused,
            router_id,
            score,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport,
            _subscription_handler: subscription_handler,
        });

        Self { inner }
    }

    /// Producer id.
    pub fn id(&self) -> ProducerId {
        self.inner.id
    }

    /// Media kind.
    pub fn kind(&self) -> MediaKind {
        self.inner.kind
    }

    /// Media kind.
    pub fn rtp_parameters(&self) -> RtpParameters {
        self.inner.rtp_parameters.clone()
    }

    /// Producer type.
    pub fn r#type(&self) -> ProducerType {
        self.inner.r#type
    }

    /// Whether the Producer is paused.
    pub fn paused(&self) -> bool {
        self.inner.paused
    }

    /// Producer score list.
    pub fn score(&self) -> Vec<ProducerScore> {
        self.inner.score.lock().unwrap().clone()
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    /// Dump Producer.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<ProducerDump, RequestError> {
        debug!("dump()");

        self.inner
            .channel
            .request(ProducerDumpRequest {
                internal: ProducerInternal {
                    router_id: self.inner.router_id,
                    transport_id: self.inner.transport.id(),
                    producer_id: self.inner.id,
                },
            })
            .await
    }

    /// Get Producer stats.
    pub async fn get_stats(&self) -> Result<Vec<ProducerStat>, RequestError> {
        debug!("get_stats()");

        self.inner
            .channel
            .request(ProducerGetStatsRequest {
                internal: ProducerInternal {
                    router_id: self.inner.router_id,
                    transport_id: self.inner.transport.id(),
                    producer_id: self.inner.id,
                },
            })
            .await
    }

    // TODO: Implement
    // /// Pause the Producer.
    // pub async fn pause(&self) -> Result<(), RequestError> {
    //     debug!("pause()");
    //
    //     // TODO: Request and update local property
    // }

    // TODO: Implement
    // /// Resume the Producer.
    // pub async fn resume(&self) -> Result<(), RequestError> {
    //     debug!("resume()");
    //
    //     // TODO: Request and update local property
    // }

    // TODO: Implement
    // /// Enable 'trace' event.
    // pub async fn  enable_trace_event(types: Vec<ProducerTraceEventType>) -> Result<(), RequestError>
    // {
    // 	debug!("enable_trace_event()");
    //
    // 	// const reqData = { types };
    //     //
    // 	// await this._channel.request(
    // 	// 	'producer.enableTraceEvent', this._internal, reqData);
    // }

    // TODO: Probably create a generic parameter on producer to make sure this method is only
    //  available when it should
    // /**
    //  * Send RTP packet (just valid for Producers created on a DirectTransport).
    //  */
    // send(rtpPacket: Buffer)
    // {
    // 	if (!Buffer.isBuffer(rtpPacket))
    // 	{
    // 		throw new TypeError('rtpPacket must be a Buffer');
    // 	}
    //
    // 	this._payloadChannel.notify(
    // 		'producer.send', this._internal, undefined, rtpPacket);
    // }

    pub fn connect_score<F: Fn(&Vec<ProducerScore>) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .score
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_video_orientation_change<F: Fn(ProducerVideoOrientation) + Send + 'static>(
        &self,
        callback: F,
    ) {
        self.inner
            .handlers
            .video_orientation_change
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_trace<F: Fn(&ProducerTraceEventData) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .trace
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    /// Consumable RTP parameters.
    pub(super) fn consumable_rtp_parameters(&self) -> RtpParameters {
        self.inner.consumable_rtp_parameters.clone()
    }
}
