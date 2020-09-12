use crate::consumer::ConsumerId;
use crate::data_structures::{
    AppData, DtlsParameters, DtlsState, IceCandidate, IceParameters, IceRole, IceState,
    NumSctpStreams, SctpParameters, SctpState, TransportInternal, TransportListenIp,
    TransportTuple, WebRtcTransportData,
};
use crate::messages::{TransportConnectRequestWebRtc, TransportConnectRequestWebRtcData};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::{Router, RouterId};
use crate::transport::{Transport, TransportId, TransportImpl};
use crate::worker::{Channel, RequestError};
use async_executor::Executor;
use async_trait::async_trait;
use log::debug;
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

#[async_trait]
impl Transport<WebRtcTransportDump, WebRtcTransportStat, WebRtcTransportRemoteParameters>
    for WebRtcTransport
{
    /// Transport id.
    fn id(&self) -> TransportId {
        self.inner.id
    }

    /// App custom data.
    fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

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
                    router_id: self.inner.router_id,
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

    async fn set_max_incoming_bitrate(&self, bitrate: u32) -> Result<(), RequestError> {
        debug!("set_max_incoming_bitrate() [bitrate:{}]", bitrate);

        self.set_max_incoming_bitrate_impl(bitrate).await
    }

    async fn produce(&self, producer_options: ProducerOptions) -> Result<Producer, RequestError> {
        self.produce_impl(producer_options).await
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
    fn router_id(&self) -> RouterId {
        self.inner.router_id
    }

    fn channel(&self) -> &Channel {
        &self.inner.channel
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

    /// ICE role.
    pub fn ice_role(&self) -> IceRole {
        return self.inner.data.ice_role;
    }

    /// ICE parameters.
    pub fn ice_parameters(&self) -> IceParameters {
        return self.inner.data.ice_parameters.clone();
    }

    /// ICE candidates.
    pub fn ice_candidates(&self) -> Vec<IceCandidate> {
        return self.inner.data.ice_candidates.clone();
    }

    /// ICE state.
    pub fn ice_state(&self) -> IceState {
        return self.inner.data.ice_state;
    }

    /// ICE selected tuple.
    pub fn ice_selected_tuple(&self) -> Option<TransportTuple> {
        return self.inner.data.ice_selected_tuple.clone();
    }

    /// DTLS parameters.
    pub fn dtls_parameters(&self) -> DtlsParameters {
        return self.inner.data.dtls_parameters.lock().unwrap().clone();
    }

    /// DTLS state.
    pub fn dtls_state(&self) -> DtlsState {
        return self.inner.data.dtls_state;
    }

    /// Remote certificate in PEM format.
    pub fn dtls_remote_cert(&self) -> Option<String> {
        return self.inner.data.dtls_remote_cert.clone();
    }

    /// SCTP parameters.
    pub fn sctp_parameters(&self) -> Option<SctpParameters> {
        return self.inner.data.sctp_parameters;
    }

    /// SCTP state.
    pub fn sctp_state(&self) -> Option<SctpState> {
        return self.inner.data.sctp_state;
    }
}
