use crate::data_structures::{
    AppData, DtlsParameters, DtlsState, IceCandidate, IceParameters, IceRole, IceState,
    NumSctpStreams, SctpParameters, SctpState, TransportListenIp, TransportTuple,
    WebRtcTransportData,
};
use crate::router::{Router, RouterId};
use crate::transport::{Transport, TransportId};
use crate::worker::Channel;
use async_executor::Executor;
use log::debug;
use std::mem;
use std::sync::{Arc, Mutex};

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

#[derive(Default)]
struct Handlers {
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

struct Inner {
    id: TransportId,
    router_id: RouterId,
    executor: Arc<Executor>,
    channel: Channel,
    payload_channel: Channel,
    handlers: Handlers,
    data: WebRtcTransportData,
    app_data: AppData,
    // Make sure router is not dropped until this transport is not dropped
    _router: Router,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        let callbacks: Vec<_> = mem::take(self.handlers.closed.lock().unwrap().as_mut());
        for callback in callbacks {
            callback();
        }

        // TODO: Fix this copy-paste
        // {
        //     let channel = self.channel.clone();
        //     let request = RouterCloseRequest {
        //         internal: RouterInternal { router_id: self.id },
        //     };
        //     self.executor
        //         .spawn(async move {
        //             if let Err(error) = channel.request(request).await {
        //                 error!("router closing failed on drop: {}", error);
        //             }
        //         })
        //         .detach();
        // }
    }
}

#[derive(Clone)]
pub struct WebRtcTransport {
    inner: Arc<Inner>,
}

impl Transport for WebRtcTransport {
    /// Transport id.
    fn id(&self) -> TransportId {
        self.inner.id
    }

    /// App custom data.
    fn app_data(&self) -> &AppData {
        &self.inner.app_data
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

impl WebRtcTransport {
    pub(super) fn new(
        id: TransportId,
        router_id: RouterId,
        executor: Arc<Executor>,
        channel: Channel,
        payload_channel: Channel,
        data: WebRtcTransportData,
        app_data: AppData,
        router: Router,
    ) -> Self {
        debug!("new()");

        let handlers = Handlers::default();
        let inner = Arc::new(Inner {
            id,
            router_id,
            executor,
            channel,
            payload_channel,
            handlers,
            data,
            app_data,
            _router: router,
        });

        Self { inner }
    }

    /**
     * ICE role.
     */
    pub fn ice_role(&self) -> IceRole {
        return self.inner.data.ice_role;
    }

    /**
     * ICE parameters.
     */
    pub fn ice_parameters(&self) -> IceParameters {
        return self.inner.data.ice_parameters.clone();
    }

    /**
     * ICE candidates.
     */
    pub fn ice_candidates(&self) -> Vec<IceCandidate> {
        return self.inner.data.ice_candidates.clone();
    }

    /**
     * ICE state.
     */
    pub fn ice_state(&self) -> IceState {
        return self.inner.data.ice_state;
    }

    /**
     * ICE selected tuple.
     */
    pub fn ice_selected_tuple(&self) -> Option<TransportTuple> {
        return self.inner.data.ice_selected_tuple.clone();
    }

    /**
     * DTLS parameters.
     */
    pub fn dtls_parameters(&self) -> DtlsParameters {
        return self.inner.data.dtls_parameters.clone();
    }

    /**
     * DTLS state.
     */
    pub fn dtls_state(&self) -> DtlsState {
        return self.inner.data.dtls_state;
    }

    /**
     * Remote certificate in PEM format.
     */
    pub fn dtls_remote_cert(&self) -> Option<String> {
        return self.inner.data.dtls_remote_cert.clone();
    }

    /**
     * SCTP parameters.
     */
    pub fn sctp_parameters(&self) -> Option<SctpParameters> {
        return self.inner.data.sctp_parameters;
    }

    /**
     * SCTP state.
     */
    pub fn sctp_state(&self) -> Option<SctpState> {
        return self.inner.data.sctp_state;
    }
}
