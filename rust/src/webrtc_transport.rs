use crate::data_structures::{
    AppData, DtlsParameters, DtlsState, IceCandidate, IceParameters, IceRole, IceState, RouterId,
    SctpParameters, SctpState, TransportId, TransportTuple, WebRtcTransportData,
};
use crate::transport::Transport;
use crate::worker::Channel;
use async_executor::Executor;
use log::debug;
use std::mem;
use std::sync::{Arc, Mutex};

#[derive(Default)]
struct Handlers {
    closed: Mutex<Vec<Box<dyn FnOnce()>>>,
}

pub struct WebRtcTransport {
    id: TransportId,
    router_id: RouterId,
    executor: Arc<Executor>,
    channel: Channel,
    payload_channel: Channel,
    handlers: Handlers,
    data: WebRtcTransportData,
    app_data: AppData,
}

impl Drop for WebRtcTransport {
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

impl Transport for WebRtcTransport {
    /// Transport id.
    fn id(&self) -> TransportId {
        self.id
    }

    /// App custom data.
    fn app_data(&self) -> &AppData {
        &self.app_data
    }

    fn connect_closed<F: FnOnce() + 'static>(&self, callback: F) {
        self.handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}

impl WebRtcTransport {
    // TODO: Ideally we'd want `pub(in super::router)`, but it doesn't work
    pub(super) fn new(
        id: TransportId,
        router_id: RouterId,
        executor: Arc<Executor>,
        channel: Channel,
        payload_channel: Channel,
        data: WebRtcTransportData,
        app_data: AppData,
    ) -> Self {
        debug!("new()");

        let handlers = Handlers::default();
        Self {
            id,
            router_id,
            executor,
            channel,
            payload_channel,
            handlers,
            data,
            app_data,
        }
    }

    /**
     * ICE role.
     */
    pub fn ice_role(&self) -> IceRole {
        return self.data.ice_role;
    }

    /**
     * ICE parameters.
     */
    pub fn ice_parameters(&self) -> IceParameters {
        return self.data.ice_parameters.clone();
    }

    /**
     * ICE candidates.
     */
    pub fn ice_candidates(&self) -> Vec<IceCandidate> {
        return self.data.ice_candidates.clone();
    }

    /**
     * ICE state.
     */
    pub fn ice_state(&self) -> IceState {
        return self.data.ice_state;
    }

    /**
     * ICE selected tuple.
     */
    pub fn ice_selected_tuple(&self) -> Option<TransportTuple> {
        return self.data.ice_selected_tuple.clone();
    }

    /**
     * DTLS parameters.
     */
    pub fn dtls_parameters(&self) -> DtlsParameters {
        return self.data.dtls_parameters.clone();
    }

    /**
     * DTLS state.
     */
    pub fn dtls_state(&self) -> DtlsState {
        return self.data.dtls_state;
    }

    /**
     * Remote certificate in PEM format.
     */
    pub fn dtls_remote_cert(&self) -> Option<String> {
        return self.data.dtls_remote_cert.clone();
    }

    /**
     * SCTP parameters.
     */
    pub fn sctp_parameters(&self) -> Option<SctpParameters> {
        return self.data.sctp_parameters;
    }

    /**
     * SCTP state.
     */
    pub fn sctp_state(&self) -> Option<SctpState> {
        return self.data.sctp_state;
    }
}
