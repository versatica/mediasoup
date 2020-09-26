use crate::data_structures::{AppData, ConsumerInternal};
use crate::messages::ConsumerCloseRequest;
use crate::producer::{ProducerId, ProducerType};
use crate::rtp_parameters::{MediaKind, RtpCapabilities, RtpParameters};
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::Channel;
use async_executor::Executor;
use log::*;
use serde::{Deserialize, Serialize};
use std::mem;
use std::sync::{Arc, Mutex};

uuid_based_wrapper_type!(ConsumerId);

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ConsumerLayers {
    /// The spatial layer index (from 0 to N).
    pub spatial_layer: u8,
    /// The temporal layer index (from 0 to N).
    pub temporal_layer: Option<u8>,
}

#[derive(Debug, Clone, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ConsumerScore {
    /// The score of the RTP stream of the consumer.
    score: u8,
    /// The score of the currently selected RTP stream of the producer.
    producer_score: u8,
    /// The scores of all RTP streams in the producer ordered by encoding (just useful when the
    /// producer uses simulcast).
    producer_scores: Vec<u8>,
}

#[derive(Debug)]
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

#[derive(Default)]
struct Handlers {
    pause: Mutex<Vec<Box<dyn Fn() + Send>>>,
    resume: Mutex<Vec<Box<dyn Fn() + Send>>>,
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

struct Inner {
    id: ConsumerId,
    producer_id: ProducerId,
    kind: MediaKind,
    r#type: ConsumerType,
    rtp_parameters: RtpParameters,
    paused: Mutex<bool>,
    executor: Arc<Executor>,
    channel: Channel,
    payload_channel: Channel,
    producer_paused: Mutex<bool>,
    priority: Mutex<u8>,
    score: Arc<Mutex<ConsumerScore>>,
    preferred_layers: Mutex<Option<ConsumerLayers>>,
    current_layers: Arc<Mutex<Option<ConsumerLayers>>>,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Box<dyn Transport>,
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
            let request = ConsumerCloseRequest {
                internal: ConsumerInternal {
                    router_id: self.transport.router_id(),
                    transport_id: self.transport.id(),
                    consumer_id: self.id,
                    producer_id: self.producer_id,
                },
            };
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("consumer closing failed on drop: {}", error);
                    }
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
    pub(super) async fn new(
        id: ConsumerId,
        producer_id: ProducerId,
        kind: MediaKind,
        r#type: ConsumerType,
        rtp_parameters: RtpParameters,
        paused: bool,
        executor: Arc<Executor>,
        channel: Channel,
        payload_channel: Channel,
        producer_paused: bool,
        score: ConsumerScore,
        preferred_layers: Option<ConsumerLayers>,
        app_data: AppData,
        transport: Box<dyn Transport>,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let score = Arc::new(Mutex::new(score));
        let current_layers = Arc::<Mutex<Option<ConsumerLayers>>>::default();
        // TODO: Fix copy-paste
        // let subscription_handler = {
        //     let handlers = Arc::clone(&handlers);
        //     let score = Arc::clone(&score);
        //
        //     channel
        //         .subscribe_to_notifications(id.to_string(), move |notification| {
        //             match serde_json::from_value::<Notification>(notification) {
        //                 Ok(notification) => match notification {
        //                     Notification::Score(scores) => {
        //                         *score.lock().unwrap() = scores.clone();
        //                         for callback in handlers.score.lock().unwrap().iter() {
        //                             callback(&scores);
        //                         }
        //                     }
        //                     Notification::VideoOrientationChange(video_orientation) => {
        //                         for callback in
        //                         handlers.video_orientation_change.lock().unwrap().iter()
        //                         {
        //                             callback(video_orientation);
        //                         }
        //                     }
        //                     Notification::Trace(trace_event_data) => {
        //                         for callback in handlers.trace.lock().unwrap().iter() {
        //                             callback(&trace_event_data);
        //                         }
        //                     }
        //                 },
        //                 Err(error) => {
        //                     error!("Failed to parse notification: {}", error);
        //                 }
        //             }
        //         })
        //         .await
        //         .unwrap()
        // };

        let inner = Arc::new(Inner {
            id,
            producer_id,
            kind,
            r#type,
            rtp_parameters,
            paused: Mutex::new(paused),
            producer_paused: Mutex::new(producer_paused),
            priority: Mutex::new(1u8),
            score,
            preferred_layers: Mutex::new(preferred_layers),
            current_layers,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport,
            // _subscription_handler: subscription_handler,
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
        *self.inner.paused.lock().unwrap()
    }

    /// Whether the associate Producer is paused.
    pub fn producer_paused(&self) -> bool {
        *self.inner.producer_paused.lock().unwrap()
    }

    /// Current priority.
    pub fn priority(&self) -> u8 {
        *self.inner.priority.lock().unwrap()
    }

    /// Consumer score.
    pub fn score(&self) -> ConsumerScore {
        self.inner.score.lock().unwrap().clone()
    }

    /// Preferred video layers.
    pub fn preferred_layers(&self) -> Option<ConsumerLayers> {
        self.inner.preferred_layers.lock().unwrap().clone()
    }

    /// Current video layers.
    pub fn current_layers(&self) -> Option<ConsumerLayers> {
        self.inner.current_layers.lock().unwrap().clone()
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    pub fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}
