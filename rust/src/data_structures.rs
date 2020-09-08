use crate::router::RouterId;
use crate::transport::TransportId;
use crate::uuid_based_wrapper_type;
use crate::webrtc_transport::WebRtcTransportOptions;
use serde::{Deserialize, Serialize};
use std::any::Any;
use std::ops::{Deref, DerefMut};
use uuid::Uuid;

#[derive(Debug)]
pub struct AppData(Box<dyn Any + Send + Sync>);

impl Default for AppData {
    fn default() -> Self {
        Self::new(())
    }
}

impl Deref for AppData {
    type Target = Box<dyn Any + Send + Sync>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

impl DerefMut for AppData {
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.0
    }
}

impl AppData {
    pub fn new<T: Any + Send + Sync>(app_data: T) -> Self {
        Self(Box::new(app_data))
    }
}

uuid_based_wrapper_type!(ConsumerId);
uuid_based_wrapper_type!(ProducerId);
uuid_based_wrapper_type!(ObserverId);

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterInternal {
    pub router_id: RouterId,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct TransportInternal {
    pub router_id: RouterId,
    pub transport_id: TransportId,
}

#[derive(Debug, Serialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct TransportListenIp {
    pub ip: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub announced_ip: Option<String>,
}

#[derive(Debug, Serialize, Copy, Clone)]
pub struct NumSctpStreams {
    /// Initially requested number of outgoing SCTP streams.
    #[serde(rename = "OS")]
    pub os: u16,
    /// Maximum number of incoming SCTP streams.
    #[serde(rename = "MIS")]
    pub mis: u16,
}

impl Default for NumSctpStreams {
    fn default() -> Self {
        Self {
            os: 1024,
            mis: 1024,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateWebrtcTransportData {
    listen_ips: Vec<TransportListenIp>,
    enable_udp: bool,
    enable_tcp: bool,
    prefer_udp: bool,
    prefer_tcp: bool,
    initial_available_outgoing_bitrate: u32,
    enable_sctp: bool,
    num_sctp_streams: NumSctpStreams,
    max_sctp_message_size: u32,
    sctp_send_buffer_size: u32,
    is_data_channel: bool,
}

impl RouterCreateWebrtcTransportData {
    pub(crate) fn from_options(webrtc_transport_options: &WebRtcTransportOptions) -> Self {
        Self {
            listen_ips: webrtc_transport_options.listen_ips.clone(),
            enable_udp: webrtc_transport_options.enable_udp,
            enable_tcp: webrtc_transport_options.enable_tcp,
            prefer_udp: webrtc_transport_options.prefer_udp,
            prefer_tcp: webrtc_transport_options.prefer_tcp,
            initial_available_outgoing_bitrate: webrtc_transport_options
                .initial_available_outgoing_bitrate,
            enable_sctp: webrtc_transport_options.enable_sctp,
            num_sctp_streams: webrtc_transport_options.num_sctp_streams,
            max_sctp_message_size: webrtc_transport_options.max_sctp_message_size,
            sctp_send_buffer_size: webrtc_transport_options.sctp_send_buffer_size,
            is_data_channel: true,
        }
    }
}

#[derive(Debug, Deserialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub enum IceRole {
    Controlled,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct IceParameters {
    username_fragment: String,
    password: String,
    ice_lite: Option<bool>,
}

#[derive(Debug, Deserialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub enum IceCandidateType {
    Host,
}

#[derive(Debug, Deserialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub enum IceCandidateTcpType {
    Passive,
}

#[derive(Debug, Deserialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub enum TransportProtocol {
    Tcp,
    Udp,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase")]
pub struct IceCandidate {
    pub foundation: String,
    pub priority: u32,
    pub ip: String,
    pub protocol: TransportProtocol,
    pub port: u16,
    pub r#type: IceCandidateType,
    pub tcp_type: Option<IceCandidateTcpType>,
}

#[derive(Debug, Deserialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub enum IceState {
    New,
    Connected,
    Completed,
    Disconnected,
    Closed,
}

#[derive(Debug, Deserialize, Clone)]
#[serde(rename_all = "camelCase", untagged)]
pub enum TransportTuple {
    LocalOnly {
        local_ip: String,
        local_port: u16,
        protocol: TransportProtocol,
    },
    WithRemote {
        local_ip: String,
        local_port: u16,
        remote_ip: String,
        remote_port: u16,
        protocol: TransportProtocol,
    },
}

#[derive(Debug, Deserialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub enum DtlsState {
    New,
    Connecting,
    Connected,
    Failed,
    Closed,
}

#[derive(Debug, Deserialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub struct SctpParameters {
    /// Must always equal 5000.
    pub port: u16,
    /// Initially requested number of outgoing SCTP streams.
    #[serde(rename = "OS")]
    pub os: u16,
    /// Maximum number of incoming SCTP streams.
    #[serde(rename = "MIS")]
    pub mis: u16,
    /// Maximum allowed size for SCTP messages.
    pub max_message_size: usize,
}

#[derive(Debug, Deserialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub enum SctpState {
    New,
    Connecting,
    Connected,
    Failed,
    Closed,
}

#[derive(Debug, Deserialize)]
pub(crate) struct WebRtcTransportData {
    pub(crate) ice_role: IceRole,
    pub(crate) ice_parameters: IceParameters,
    pub(crate) ice_candidates: Vec<IceCandidate>,
    pub(crate) ice_state: IceState,
    pub(crate) ice_selected_tuple: Option<TransportTuple>,
    pub(crate) dtls_parameters: DtlsParameters,
    pub(crate) dtls_state: DtlsState,
    pub(crate) dtls_remote_cert: Option<String>,
    pub(crate) sctp_parameters: Option<SctpParameters>,
    pub(crate) sctp_state: Option<SctpState>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreatePlainTransportData {
    pub listen_ip: TransportListenIp,
    pub rtcp_mux: bool,
    pub comedia: bool,
    pub enable_sctp: bool,
    pub num_sctp_streams: NumSctpStreams,
    pub max_sctp_message_size: u32,
    pub sctp_send_buffer_size: u32,
    pub enable_srtp: bool,
    pub srtp_crypto_suite: String,
    is_data_channel: bool,
}

impl RouterCreatePlainTransportData {
    pub fn new(listen_ip: TransportListenIp) -> Self {
        Self {
            listen_ip,
            rtcp_mux: true,
            comedia: false,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 262144,
            sctp_send_buffer_size: 262144,
            enable_srtp: false,
            srtp_crypto_suite: "AES_CM_128_HMAC_SHA1_80".to_string(),
            is_data_channel: false,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreatePipeTransportData {
    pub listen_ip: TransportListenIp,
    pub enable_sctp: bool,
    pub num_sctp_streams: NumSctpStreams,
    pub max_sctp_message_size: u32,
    pub sctp_send_buffer_size: u32,
    pub enable_rtx: bool,
    pub enable_srtp: bool,
    is_data_channel: bool,
}

impl RouterCreatePipeTransportData {
    pub fn new(listen_ip: TransportListenIp) -> Self {
        Self {
            listen_ip,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 268435456,
            sctp_send_buffer_size: 268435456,
            enable_rtx: false,
            enable_srtp: false,
            is_data_channel: false,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreateDirectTransportData {
    pub direct: bool,
    pub max_message_size: u32,
}

impl RouterCreateDirectTransportData {
    pub fn new() -> Self {
        Self {
            direct: true,
            max_message_size: 262144,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub(crate) struct RouterCreateAudioLevelObserverInternal {
    pub router_id: RouterId,
    pub rtp_observer_id: Uuid,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RouterCreateAudioLevelObserverData {
    pub max_entries: u16,
    pub threshold: i8,
    pub interval: u16,
}

impl RouterCreateAudioLevelObserverData {
    pub fn new() -> Self {
        Self {
            max_entries: 1,
            threshold: -80,
            interval: 1000,
        }
    }
}

#[derive(Debug, Deserialize, Serialize, Copy, Clone)]
#[serde(rename_all = "camelCase")]
pub enum DtlsRole {
    Auto,
    Client,
    Server,
}

impl Default for DtlsRole {
    fn default() -> Self {
        Self::Auto
    }
}

#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct DtlsFingerprint {
    pub algorithm: String,
    pub value: String,
}

#[derive(Debug, Deserialize, Serialize, Clone)]
pub struct DtlsParameters {
    pub role: DtlsRole,
    pub fingerprints: Vec<DtlsFingerprint>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct TransportConnectData {
    pub dtls_parameters: DtlsParameters,
}

impl TransportConnectData {
    pub fn new(dtls_parameters: DtlsParameters) -> Self {
        Self { dtls_parameters }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct TransportSetMaxIncomingBitrateData {
    pub bitrate: u64,
}

impl TransportSetMaxIncomingBitrateData {
    pub fn new(bitrate: u64) -> Self {
        Self { bitrate }
    }
}
