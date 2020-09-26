use crate::data_structures::AppData;
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
    r#type: ProducerType,
    rtp_parameters: RtpParameters,
    paused: Mutex<bool>,
    executor: Arc<Executor>,
    channel: Channel,
    payload_channel: Channel,
    producer_paused: Mutex<bool>,
    score: Arc<Mutex<ConsumerScore>>,
    preferred_layers: Mutex<Option<ConsumerLayers>>,
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

        // TODO: Fix copy-paste
        // {
        //     let channel = self.channel.clone();
        //     let request = ConsumerCloseRequest {
        //         internal: ConsumerInternal {
        //             router_id: self.router_id,
        //             transport_id: self.transport.id(),
        //             producer_id: self.id,
        //         },
        //     };
        //     self.executor
        //         .spawn(async move {
        //             if let Err(error) = channel.request(request).await {
        //                 error!("producer closing failed on drop: {}", error);
        //             }
        //         })
        //         .detach();
        // }
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
        r#type: ProducerType,
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
            score,
            preferred_layers: Mutex::new(preferred_layers),
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

    pub fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}
