use crate::consumer::ConsumerId;
use crate::data_structures::{
    AppData, DtlsParameters, DtlsState, IceCandidate, IceParameters, IceRole, IceState,
    NumSctpStreams, SctpParameters, SctpState, TransportInternal, TransportListenIp,
    TransportTuple, WebRtcTransportData,
};
use crate::messages::{
    TransportCloseRequest, TransportConnectRequestWebRtc, TransportConnectRequestWebRtcData,
    TransportRestartIceRequest,
};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::transport::{ProduceError, TransportTraceEventData};
use crate::router::Router;
use crate::transport::{Transport, TransportGeneric, TransportId, TransportImpl};
use crate::worker::{Channel, RequestError, SubscriptionHandler};
use async_executor::Executor;
use async_trait::async_trait;
use log::debug;
use log::error;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::convert::TryFrom;
use std::mem;
use std::ops::Deref;
use std::sync::{Arc, Mutex};
use thiserror::Error;

/// Struct that protects an invariant of having non-empty list of listen IPs
#[derive(Debug, Serialize, Clone)]
pub struct TransportListenIps(Vec<TransportListenIp>);

impl TransportListenIps {
    pub fn new(listen_ip: TransportListenIp) -> Self {
        Self(vec![listen_ip])
    }

    pub fn add(mut self, listen_ip: TransportListenIp) -> Self {
        self.0.push(listen_ip);
        self
    }
}

impl Deref for TransportListenIps {
    type Target = Vec<TransportListenIp>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

#[derive(Error, Debug)]
#[error("Empty list of listen IPs provided, should have at least one element")]
pub struct EmptyListError;

impl TryFrom<Vec<TransportListenIp>> for TransportListenIps {
    type Error = EmptyListError;

    fn try_from(listen_ips: Vec<TransportListenIp>) -> Result<Self, Self::Error> {
        if listen_ips.is_empty() {
            Err(EmptyListError)
        } else {
            Ok(Self(listen_ips))
        }
    }
}

#[derive(Debug)]
#[non_exhaustive]
pub struct WebRtcTransportOptions {
    pub listen_ips: TransportListenIps,
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
    pub fn new(listen_ips: TransportListenIps) -> Self {
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

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct RtpListener {
    // TODO: What is this field format?
    pub mid_table: HashMap<(), ()>,
    // TODO: What is this field format?
    pub rid_table: HashMap<(), ()>,
    // TODO: What is this field format?
    pub ssrc_table: HashMap<(), ()>,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WebRtcTransportDump {
    pub id: TransportId,
    pub consumer_ids: Vec<ConsumerId>,
    pub producer_ids: Vec<ProducerId>,
    pub data_consumer_ids: Vec<ConsumerId>,
    pub data_producer_ids: Vec<ProducerId>,
    pub dtls_parameters: DtlsParameters,
    pub dtls_state: DtlsState,
    pub ice_candidates: Vec<IceCandidate>,
    pub ice_parameters: IceParameters,
    pub ice_role: IceRole,
    pub ice_state: IceState,
    // TODO: What is this field format?
    pub map_rtx_ssrc_consumer_id: HashMap<(), ()>,
    // TODO: What is this field format?
    pub map_ssrc_consumer_id: HashMap<(), ()>,
    pub max_message_size: usize,
    // TODO: What is this field format (returned `{}`, but it seems it should be a Vec)?
    pub recv_rtp_header_extensions: HashMap<(), ()>,
    pub rtp_listener: RtpListener,
    // TODO: What is this field format?
    pub trace_event_types: String,
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "kebab-case")]
pub enum WebRtcTransportStatType {
    WebrtcTransport,
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct WebRtcTransportStat {
    // Common to all Transports.
    pub r#type: WebRtcTransportStatType,
    pub transport_id: TransportId,
    pub timestamp: u32,
    pub sctp_state: Option<SctpState>,
    pub bytes_received: usize,
    pub recv_bitrate: u32,
    pub bytes_sent: usize,
    pub send_bitrate: u32,
    pub rtp_bytes_received: usize,
    pub rtp_recv_bitrate: u32,
    pub rtp_bytes_sent: usize,
    pub rtp_send_bitrate: u32,
    pub rtx_bytes_received: usize,
    pub rtx_recv_bitrate: u32,
    pub rtx_bytes_sent: usize,
    pub rtx_send_bitrate: u32,
    pub probation_bytes_sent: usize,
    pub probation_send_bitrate: u32,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub available_outgoing_bitrate: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub available_incoming_bitrate: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub max_incoming_bitrate: Option<u32>,
    // WebRtcTransport specific.
    pub ice_role: IceRole,
    pub ice_state: IceState,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub ice_selected_tuple: Option<TransportTuple>,
    pub dtls_state: DtlsState,
}

pub struct WebRtcTransportRemoteParameters {
    pub dtls_parameters: DtlsParameters,
}

#[derive(Default)]
struct Handlers {
    ice_state_change: Mutex<Vec<Box<dyn Fn(IceState) + Send>>>,
    ice_selected_tuple_change: Mutex<Vec<Box<dyn Fn(&TransportTuple) + Send>>>,
    dtls_state_change: Mutex<Vec<Box<dyn Fn(DtlsState) + Send>>>,
    sctp_state_change: Mutex<Vec<Box<dyn Fn(SctpState) + Send>>>,
    trace: Mutex<Vec<Box<dyn Fn(&TransportTraceEventData) + Send>>>,
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    #[serde(rename_all = "camelCase")]
    IceStateChange { ice_state: IceState },
    #[serde(rename_all = "camelCase")]
    IceSelectedTupleChange { ice_selected_tuple: TransportTuple },
    #[serde(rename_all = "camelCase")]
    DtlsStateChange {
        dtls_state: DtlsState,
        dtls_remote_cert: Option<String>,
    },
    #[serde(rename_all = "camelCase")]
    SctpStateChange { sctp_state: SctpState },
    #[serde(rename_all = "camelCase")]
    Trace(TransportTraceEventData),
}

struct Inner {
    id: TransportId,
    executor: Arc<Executor>,
    channel: Channel,
    payload_channel: Channel,
    handlers: Arc<Handlers>,
    data: Arc<WebRtcTransportData>,
    app_data: AppData,
    // Make sure router is not dropped until this transport is not dropped
    router: Router,
    // Drop subscription to transport-specific notifications when transport itself is dropped
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
            let request = TransportCloseRequest {
                internal: TransportInternal {
                    router_id: self.router.id(),
                    transport_id: self.id,
                },
            };
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("transport closing failed on drop: {}", error);
                    }
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct WebRtcTransport {
    inner: Arc<Inner>,
}

#[async_trait(?Send)]
impl Transport for WebRtcTransport {
    /// Transport id.
    fn id(&self) -> TransportId {
        self.inner.id
    }

    /// App custom data.
    fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    /// Set maximum incoming bitrate for receiving media.
    async fn set_max_incoming_bitrate(&self, bitrate: u32) -> Result<(), RequestError> {
        debug!("set_max_incoming_bitrate() [bitrate:{}]", bitrate);

        self.set_max_incoming_bitrate_impl(bitrate).await
    }

    /// Create a Producer.
    ///
    /// Transport will be kept alive as long as at least one producer instance is alive.
    async fn produce(&self, producer_options: ProducerOptions) -> Result<Producer, ProduceError> {
        debug!("produce()");

        self.produce_impl(producer_options).await
    }
}

#[async_trait(?Send)]
impl TransportGeneric<WebRtcTransportDump, WebRtcTransportStat, WebRtcTransportRemoteParameters>
    for WebRtcTransport
{
    /// Dump Transport.
    async fn dump(&self) -> Result<WebRtcTransportDump, RequestError> {
        debug!("dump()");

        self.dump_impl().await
    }

    /// Get Transport stats.
    async fn get_stats(&self) -> Result<Vec<WebRtcTransportStat>, RequestError> {
        debug!("get_stats()");

        self.get_stats_impl().await
    }

    /// Provide the WebRtcTransport remote parameters.
    async fn connect(
        &self,
        remote_parameters: WebRtcTransportRemoteParameters,
    ) -> Result<(), RequestError> {
        debug!("connect()");

        let response = self
            .inner
            .channel
            .request(TransportConnectRequestWebRtc {
                internal: TransportInternal {
                    router_id: self.inner.router.id(),
                    transport_id: self.inner.id,
                },
                data: TransportConnectRequestWebRtcData {
                    dtls_parameters: remote_parameters.dtls_parameters,
                },
            })
            .await?;

        self.inner.data.dtls_parameters.lock().unwrap().role = response.dtls_local_role;

        Ok(())
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

impl TransportImpl<WebRtcTransportDump, WebRtcTransportStat, WebRtcTransportRemoteParameters>
    for WebRtcTransport
{
    fn router(&self) -> &Router {
        &self.inner.router
    }

    fn channel(&self) -> &Channel {
        &self.inner.channel
    }

    fn payload_channel(&self) -> &Channel {
        &self.inner.payload_channel
    }

    fn has_producer(&self, id: &ProducerId) -> bool {
        self.inner.router.has_producer(id)
    }

    fn executor(&self) -> &Arc<Executor> {
        &self.inner.executor
    }
}

impl WebRtcTransport {
    pub(super) async fn new(
        id: TransportId,
        executor: Arc<Executor>,
        channel: Channel,
        payload_channel: Channel,
        data: WebRtcTransportData,
        app_data: AppData,
        router: Router,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let data = Arc::new(data);

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);
            let data = Arc::clone(&data);

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    match serde_json::from_value::<Notification>(notification) {
                        Ok(notification) => match notification {
                            Notification::IceStateChange { ice_state } => {
                                *data.ice_state.lock().unwrap() = ice_state;
                                for callback in handlers.ice_state_change.lock().unwrap().iter() {
                                    callback(ice_state);
                                }
                            }
                            Notification::IceSelectedTupleChange { ice_selected_tuple } => {
                                data.ice_selected_tuple
                                    .lock()
                                    .unwrap()
                                    .replace(ice_selected_tuple.clone());
                                for callback in
                                    handlers.ice_selected_tuple_change.lock().unwrap().iter()
                                {
                                    callback(&ice_selected_tuple);
                                }
                            }
                            Notification::DtlsStateChange {
                                dtls_state,
                                dtls_remote_cert,
                            } => {
                                *data.dtls_state.lock().unwrap() = dtls_state;

                                if let Some(dtls_remote_cert) = dtls_remote_cert {
                                    data.dtls_remote_cert
                                        .lock()
                                        .unwrap()
                                        .replace(dtls_remote_cert);
                                }

                                for callback in handlers.dtls_state_change.lock().unwrap().iter() {
                                    callback(dtls_state);
                                }
                            }
                            Notification::SctpStateChange { sctp_state } => {
                                data.sctp_state.lock().unwrap().replace(sctp_state);

                                for callback in handlers.sctp_state_change.lock().unwrap().iter() {
                                    callback(sctp_state);
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
            executor,
            channel,
            payload_channel,
            handlers,
            data,
            app_data,
            router,
            _subscription_handler: subscription_handler,
        });

        Self { inner }
    }

    /// ICE role.
    pub fn ice_role(&self) -> IceRole {
        self.inner.data.ice_role
    }

    /// ICE parameters.
    pub fn ice_parameters(&self) -> IceParameters {
        self.inner.data.ice_parameters.clone()
    }

    /// ICE candidates.
    pub fn ice_candidates(&self) -> Vec<IceCandidate> {
        self.inner.data.ice_candidates.clone()
    }

    /// ICE state.
    pub fn ice_state(&self) -> IceState {
        *self.inner.data.ice_state.lock().unwrap()
    }

    /// ICE selected tuple.
    pub fn ice_selected_tuple(&self) -> Option<TransportTuple> {
        self.inner.data.ice_selected_tuple.lock().unwrap().clone()
    }

    /// DTLS parameters.
    pub fn dtls_parameters(&self) -> DtlsParameters {
        self.inner.data.dtls_parameters.lock().unwrap().clone()
    }

    /// DTLS state.
    pub fn dtls_state(&self) -> DtlsState {
        *self.inner.data.dtls_state.lock().unwrap()
    }

    /// Remote certificate in PEM format.
    pub fn dtls_remote_cert(&self) -> Option<String> {
        self.inner.data.dtls_remote_cert.lock().unwrap().clone()
    }

    /// SCTP parameters.
    pub fn sctp_parameters(&self) -> Option<SctpParameters> {
        self.inner.data.sctp_parameters
    }

    /// SCTP state.
    pub fn sctp_state(&self) -> Option<SctpState> {
        *self.inner.data.sctp_state.lock().unwrap()
    }

    /// Restart ICE.
    pub async fn restart_ice(&self) -> Result<IceParameters, RequestError> {
        debug!("restart_ice()");

        let response = self
            .inner
            .channel
            .request(TransportRestartIceRequest {
                internal: TransportInternal {
                    router_id: self.inner.router.id(),
                    transport_id: self.inner.id,
                },
            })
            .await?;

        Ok(response.ice_parameters)
    }

    pub fn connect_ice_state_change<F: Fn(IceState) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .ice_state_change
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_ice_selected_tuple_change<F: Fn(&TransportTuple) + Send + 'static>(
        &self,
        callback: F,
    ) {
        self.inner
            .handlers
            .ice_selected_tuple_change
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_dtls_state_change<F: Fn(DtlsState) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .dtls_state_change
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_sctp_state_change<F: Fn(SctpState) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .sctp_state_change
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_trace<F: Fn(&TransportTraceEventData) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .trace
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}
