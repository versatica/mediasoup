use crate::data_structures::{
    AppData, ConsumerId, NumSctpStreams, ObserverId, ProducerId, RouterCreateWebrtcTransportData,
    RouterId, RouterInternal, TransportId, TransportInternal, TransportListenIp,
};
use crate::messages::{RouterCloseRequest, RouterCreateWebrtcTransportRequest, RouterDumpRequest};
use crate::ortc::RtpCapabilities;
use crate::transport::Transport;
use crate::webrtc_transport::WebRtcTransport;
use crate::worker::{Channel, RequestError};
use async_executor::Executor;
use log::debug;
use log::error;
use serde::Deserialize;
use std::collections::{HashMap, HashSet};
use std::mem;
use std::sync::{Arc, Mutex, Weak};

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

#[derive(Debug)]
pub struct WebRtcTransportOptions {
    pub listen_ips: Vec<TransportListenIp>,
    pub enable_udp: bool,
    pub enable_tcp: bool,
    pub prefer_udp: bool,
    pub prefer_tcp: bool,
    pub initial_available_outgoing_bitrate: u32,
    pub enable_sctp: bool,
    pub num_sctp_streams: NumSctpStreams,
    pub max_sctp_message_size: u32,
    pub sctp_send_buffer_size: u32,
    pub app_data: AppData,
}

impl WebRtcTransportOptions {
    pub fn new(listen_ips: Vec<TransportListenIp>) -> Self {
        Self {
            listen_ips,
            enable_udp: true,
            enable_tcp: false,
            prefer_udp: false,
            prefer_tcp: false,
            initial_available_outgoing_bitrate: 600_000,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 262144,
            sctp_send_buffer_size: 262144,
            app_data: AppData::default(),
        }
    }
}

pub enum NewTransport<'a> {
    WebRtc(&'a Arc<WebRtcTransport>),
}

enum WeakTransports {
    WebRtc(Weak<WebRtcTransport>),
}

#[derive(Default)]
struct Handlers {
    new_transport: Mutex<Vec<Box<dyn Fn(NewTransport)>>>,
    closed: Mutex<Vec<Box<dyn FnOnce()>>>,
}

pub struct Router {
    id: RouterId,
    executor: Arc<Executor>,
    rtp_capabilities: RtpCapabilities,
    channel: Channel,
    payload_channel: Channel,
    transports: Arc<Mutex<HashMap<TransportId, WeakTransports>>>,
    handlers: Handlers,
    app_data: AppData,
}

impl Drop for Router {
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

impl Router {
    // TODO: Ideally we'd want `pub(in super::worker)`, but it doesn't work
    pub(super) fn new(
        id: RouterId,
        executor: Arc<Executor>,
        rtp_capabilities: RtpCapabilities,
        channel: Channel,
        payload_channel: Channel,
        app_data: AppData,
    ) -> Self {
        debug!("new()");

        let transports = Arc::default();
        let handlers = Handlers::default();
        Self {
            id,
            executor,
            rtp_capabilities,
            channel,
            payload_channel,
            transports,
            handlers,
            app_data,
        }
    }

    /// Router id.
    pub fn id(&self) -> RouterId {
        self.id
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.app_data
    }

    /// RTC capabilities of the Router.
    pub fn rtp_capabilities(&self) -> RtpCapabilities {
        self.rtp_capabilities.clone()
    }

    /// Dump Router.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<RouterDumpResponse, RequestError> {
        debug!("dump()");

        self.channel
            .request(RouterDumpRequest {
                internal: RouterInternal { router_id: self.id },
            })
            .await
    }

    /// Create a WebRtcTransport.
    pub async fn create_webrtc_transport(
        &self,
        webrtc_transport_options: WebRtcTransportOptions,
    ) -> Result<Arc<WebRtcTransport>, RequestError> {
        debug!("create_webrtc_transport()");

        let transport_id = TransportId::new();
        let data = self
            .channel
            .request(RouterCreateWebrtcTransportRequest {
                internal: TransportInternal {
                    router_id: self.id,
                    transport_id,
                },
                data: RouterCreateWebrtcTransportData::from_options(&webrtc_transport_options),
            })
            .await?;

        let transport = Arc::new(WebRtcTransport::new(
            transport_id,
            self.id,
            Arc::clone(&self.executor),
            self.channel.clone(),
            self.payload_channel.clone(),
            data,
            webrtc_transport_options.app_data,
        ));

        let transports = self.transports.clone();
        transports.lock().unwrap().insert(
            transport_id,
            WeakTransports::WebRtc(Arc::downgrade(&transport)),
        );
        transport.connect_closed(move || {
            transports.lock().unwrap().remove(&transport_id);
        });
        for callback in self.handlers.new_transport.lock().unwrap().iter() {
            callback(NewTransport::WebRtc(&transport));
        }

        Ok(transport)
    }

    pub fn connect_new_transport<F: Fn(NewTransport) + 'static>(&self, callback: F) {
        self.handlers
            .new_transport
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_closed<F: FnOnce() + 'static>(&self, callback: F) {
        self.handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}
