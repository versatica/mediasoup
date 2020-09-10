pub mod transport;
pub mod webrtc_transport;
use crate::uuid_based_wrapper_type;

use crate::data_structures::{
    AppData, ConsumerId, ObserverId, ProducerId, RouterCreateWebrtcTransportData, RouterInternal,
    TransportInternal,
};
use crate::messages::{RouterCloseRequest, RouterCreateWebrtcTransportRequest, RouterDumpRequest};
use crate::ortc::RtpCapabilities;
use crate::transport::TransportId;
use crate::webrtc_transport::{WebRtcTransport, WebRtcTransportOptions};
use crate::worker::{Channel, RequestError, Worker};
use async_executor::Executor;
use log::debug;
use log::error;
use serde::Deserialize;
use std::collections::{HashMap, HashSet};
use std::mem;
use std::sync::{Arc, Mutex};

uuid_based_wrapper_type!(RouterId);

#[derive(Debug, Default)]
pub struct RouterOptions {
    rtp_capabilities: RtpCapabilities,
    app_data: AppData,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RouterDumpResponse {
    id: RouterId,
    map_consumer_id_producer_id: HashMap<ConsumerId, ProducerId>,
    map_data_consumer_id_data_producer_id: HashMap<ConsumerId, ProducerId>,
    map_data_producer_id_data_consumer_ids: HashMap<ProducerId, HashSet<ConsumerId>>,
    map_producer_id_consumer_ids: HashMap<ProducerId, HashSet<ConsumerId>>,
    map_producer_id_observer_ids: HashMap<ProducerId, HashSet<ObserverId>>,
    rtp_observer_ids: HashSet<ObserverId>,
    transport_ids: HashSet<TransportId>,
}

pub enum NewTransport<'a> {
    WebRtc(&'a Arc<WebRtcTransport>),
}

#[derive(Default)]
struct Handlers {
    new_transport: Mutex<Vec<Box<dyn Fn(NewTransport) + Send>>>,
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

struct Inner {
    id: RouterId,
    executor: Arc<Executor>,
    rtp_capabilities: RtpCapabilities,
    channel: Channel,
    payload_channel: Channel,
    handlers: Handlers,
    app_data: AppData,
    // Make sure worker is not dropped until this router is not dropped
    _worker: Worker,
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
            let request = RouterCloseRequest {
                internal: RouterInternal { router_id: self.id },
            };
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("router closing failed on drop: {}", error);
                    }
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct Router {
    inner: Arc<Inner>,
}

impl Router {
    pub(super) fn new(
        id: RouterId,
        executor: Arc<Executor>,
        channel: Channel,
        payload_channel: Channel,
        RouterOptions {
            app_data,
            rtp_capabilities,
        }: RouterOptions,
        worker: Worker,
    ) -> Self {
        debug!("new()");

        let handlers = Handlers::default();
        let inner = Arc::new(Inner {
            id,
            executor,
            rtp_capabilities,
            channel,
            payload_channel,
            handlers,
            app_data,
            _worker: worker,
        });

        Self { inner }
    }

    /// Router id.
    pub fn id(&self) -> RouterId {
        self.inner.id
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    /// RTC capabilities of the Router.
    pub fn rtp_capabilities(&self) -> RtpCapabilities {
        self.inner.rtp_capabilities.clone()
    }

    /// Dump Router.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<RouterDumpResponse, RequestError> {
        debug!("dump()");

        self.inner
            .channel
            .request(RouterDumpRequest {
                internal: RouterInternal {
                    router_id: self.inner.id,
                },
            })
            .await
    }

    /// Create a WebRtcTransport.
    ///
    /// Router will be kept alive as long as at least one transport is alive.
    pub async fn create_webrtc_transport(
        &self,
        webrtc_transport_options: WebRtcTransportOptions,
    ) -> Result<Arc<WebRtcTransport>, RequestError> {
        debug!("create_webrtc_transport()");

        let transport_id = TransportId::new();
        let data = self
            .inner
            .channel
            .request(RouterCreateWebrtcTransportRequest {
                internal: TransportInternal {
                    router_id: self.inner.id,
                    transport_id,
                },
                data: RouterCreateWebrtcTransportData::from_options(&webrtc_transport_options),
            })
            .await?;

        let transport = Arc::new(WebRtcTransport::new(
            transport_id,
            self.inner.id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            self.inner.payload_channel.clone(),
            data,
            webrtc_transport_options.app_data,
            self.clone(),
        ));

        for callback in self.inner.handlers.new_transport.lock().unwrap().iter() {
            callback(NewTransport::WebRtc(&transport));
        }
        // TODO: Subscribe when added on transport:
        //  connect_new_producer
        //  connect_producer_closed
        //  connect_new_data_producer
        //  connect_data_producer_closed

        Ok(transport)
    }

    pub fn connect_new_transport<F: Fn(NewTransport) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .new_transport
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
}
