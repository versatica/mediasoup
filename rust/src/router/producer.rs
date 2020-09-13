use crate::data_structures::{AppData, ProducerInternal};
use crate::messages::ProducerCloseRequest;
use crate::router::RouterId;
use crate::rtp_parameters::{MediaKind, RtpParameters};
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::{Channel, RequestError};
use async_executor::Executor;
use log::{debug, error};
use serde::{Deserialize, Serialize};
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
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

struct Inner {
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
    pub(super) fn new(
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

        // TODO: Subscribe to notifications

        let inner = Arc::new(Inner {
            id,
            kind,
            r#type,
            rtp_parameters,
            consumable_rtp_parameters,
            paused,
            router_id,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport,
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

    // TODO: Implement
    // /// Producer score list.
    // pub fn score(&self) -> Vec<ProducerScore> {
    //     self.inner.score
    // }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    // TODO: Implement
    // /// Dump Producer.
    // pub async fn dump(&self) -> Result<ProducerDump, RequestError> {
    //     debug!("dump()");
    //
    //     // TODO: Request
    // }

    // TODO: Implement
    // /// Get Producer stats.
    // pub async fn get_stats(&self) -> Result<Vec<ProducerStat>, RequestError> {
    //     debug!("get_stats()");
    //
    //     // TODO: Request
    // }

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

    /// Consumable RTP parameters.
    pub(super) fn consumable_rtp_parameters(&self) -> RtpParameters {
        self.inner.consumable_rtp_parameters.clone()
    }

    fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}
