use bytes::Bytes;
use serde::{Deserialize, Serialize};
use std::any::Any;
use std::net::IpAddr;
use std::ops::{Deref, DerefMut};
use std::sync::Arc;

#[derive(Debug, Clone)]
pub struct AppData(Arc<dyn Any + Send + Sync>);

impl Default for AppData {
    fn default() -> Self {
        Self::new(())
    }
}

impl Deref for AppData {
    type Target = Arc<dyn Any + Send + Sync>;

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
        Self(Arc::new(app_data))
    }
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct TransportListenIp {
    pub ip: IpAddr,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub announced_ip: Option<IpAddr>,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum IceRole {
    Controlled,
    Controlling,
}

#[derive(Debug, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct IceParameters {
    pub username_fragment: String,
    pub password: String,
    pub ice_lite: Option<bool>,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum IceCandidateType {
    Host,
    Srflx,
    Prflx,
    Relay,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum IceCandidateTcpType {
    Passive,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum TransportProtocol {
    Tcp,
    Udp,
}

#[derive(Debug, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct IceCandidate {
    pub foundation: String,
    pub priority: u32,
    pub ip: IpAddr,
    pub protocol: TransportProtocol,
    pub port: u16,
    pub r#type: IceCandidateType,
    pub tcp_type: Option<IceCandidateTcpType>,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum IceState {
    New,
    Connected,
    Completed,
    Disconnected,
    Closed,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(untagged)]
pub enum TransportTuple {
    #[serde(rename_all = "camelCase")]
    WithRemote {
        local_ip: IpAddr,
        local_port: u16,
        remote_ip: IpAddr,
        remote_port: u16,
        protocol: TransportProtocol,
    },
    #[serde(rename_all = "camelCase")]
    LocalOnly {
        local_ip: IpAddr,
        local_port: u16,
        protocol: TransportProtocol,
    },
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum DtlsState {
    New,
    Connecting,
    Connected,
    Failed,
    Closed,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum SctpState {
    New,
    Connecting,
    Connected,
    Failed,
    Closed,
}

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
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

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
pub struct DtlsFingerprint {
    // TODO: Algorithm should likely be an enum, or the whole DtlsFingerprint an enum with only
    //  known algorithms supported and fixed size for fingerprints
    pub algorithm: String,
    pub value: String,
}

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
pub struct DtlsParameters {
    pub role: DtlsRole,
    pub fingerprints: Vec<DtlsFingerprint>,
}

#[derive(Debug, Copy, Clone, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum EventDirection {
    In,
    Out,
}

#[derive(Clone)]
pub enum WebRtcMessage {
    String(String),
    Binary(Bytes),
    EmptyString,
    EmptyBinary,
}

impl WebRtcMessage {
    // +------------------------------------+-----------+
    // | Value                              | SCTP PPID |
    // +------------------------------------+-----------+
    // | WebRTC String                      | 51        |
    // | WebRTC Binary Partial (Deprecated) | 52        |
    // | WebRTC Binary                      | 53        |
    // | WebRTC String Partial (Deprecated) | 54        |
    // | WebRTC String Empty                | 56        |
    // | WebRTC Binary Empty                | 57        |
    // +------------------------------------+-----------+

    pub(crate) fn new(ppid: u32, payload: Bytes) -> Self {
        // TODO: Make this fallible instead
        match ppid {
            51 => WebRtcMessage::String(String::from_utf8(payload.to_vec()).unwrap()),
            53 => WebRtcMessage::Binary(payload),
            56 => WebRtcMessage::EmptyString,
            57 => WebRtcMessage::EmptyBinary,
            _ => {
                panic!("Bad ppid {}", ppid);
            }
        }
    }

    pub(crate) fn into_ppid_and_payload(self) -> (u32, Bytes) {
        match self {
            WebRtcMessage::String(string) => (51_u32, Bytes::from(string)),
            WebRtcMessage::Binary(binary) => (53_u32, binary),
            WebRtcMessage::EmptyString => (56_u32, Bytes::from_static(b" ")),
            WebRtcMessage::EmptyBinary => (57_u32, Bytes::from(vec![0u8])),
        }
    }
}
