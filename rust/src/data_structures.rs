//! Miscellaneous data structures.

#[cfg(test)]
mod tests;

use serde::de::{MapAccess, Visitor};
use serde::ser::SerializeStruct;
use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
use std::any::Any;
use std::borrow::Cow;
use std::fmt;
use std::net::IpAddr;
use std::ops::{Deref, DerefMut};
use std::sync::Arc;

/// Container for arbitrary data attached to mediasoup entities.
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
    /// Construct app data from almost anything
    pub fn new<T: Any + Send + Sync>(app_data: T) -> Self {
        Self(Arc::new(app_data))
    }
}

/// IP to listen on.
///
/// # Notes on usage
/// If you use "0.0.0.0" or "::" as ip value, then you need to also provide `announced_ip`.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct ListenIp {
    /// Listening IPv4 or IPv6.
    pub ip: IpAddr,
    /// Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with private IP).
    #[serde(skip_serializing_if = "Option::is_none")]
    pub announced_ip: Option<IpAddr>,
}

/// ICE role.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum IceRole {
    /// The transport is the controlled agent.
    Controlled,
    /// The transport is the controlling agent.
    Controlling,
}

/// ICE parameters.
#[derive(Debug, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct IceParameters {
    /// ICE username fragment.
    pub username_fragment: String,
    /// ICE password.
    pub password: String,
    /// ICE Lite.
    pub ice_lite: Option<bool>,
}

/// ICE candidate type.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum IceCandidateType {
    /// The candidate is a host candidate, whose IP address as specified in the
    /// [`IceCandidate::ip`] property is in fact the true address of the remote peer.
    Host,
    /// The candidate is a server reflexive candidate; the [`IceCandidate::ip`] indicates an
    /// intermediary address assigned by the STUN server to represent the candidate's peer
    /// anonymously.
    Srflx,
    /// The candidate is a peer reflexive candidate; the [`IceCandidate::ip`] is an intermediary
    /// address assigned by the STUN server to represent the candidate's peer anonymously.
    Prflx,
    /// The candidate is a relay candidate, obtained from a TURN server. The relay candidate's IP
    /// address is an address the [TURN](https://developer.mozilla.org/en-US/docs/Glossary/TURN)
    /// server uses to forward the media between the two peers.
    Relay,
}

/// ICE candidate TCP type (always `Passive`).
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum IceCandidateTcpType {
    /// Passive.
    Passive,
}

/// Transport protocol.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum Protocol {
    /// TCP.
    Tcp,
    /// UDP.
    Udp,
}

/// ICE candidate
#[derive(Debug, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct IceCandidate {
    /// Unique identifier that allows ICE to correlate candidates that appear on multiple
    /// transports.
    pub foundation: String,
    /// The assigned priority of the candidate.
    pub priority: u32,
    /// The IP address of the candidate.
    pub ip: IpAddr,
    /// The protocol of the candidate.
    pub protocol: Protocol,
    /// The port for the candidate.
    pub port: u16,
    /// The type of candidate (always `Host`).
    pub r#type: IceCandidateType,
    /// The type of TCP candidate (always `Passive`).
    #[serde(skip_serializing_if = "Option::is_none")]
    pub tcp_type: Option<IceCandidateTcpType>,
}

/// ICE state.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum IceState {
    /// No ICE Binding Requests have been received yet.
    New,
    /// Valid ICE Binding Request have been received, but none with USE-CANDIDATE attribute.
    /// Outgoing media is allowed.
    Connected,
    /// ICE Binding Request with USE_CANDIDATE attribute has been received. Media in both directions
    /// is now allowed.
    Completed,
    /// ICE was `Connected` or `Completed` but it has suddenly failed (this can just happen if the
    /// selected tuple has `Tcp` protocol).
    Disconnected,
    /// ICE state when the transport has been closed.
    Closed,
}

/// Tuple of local IP/port/protocol + optional remote IP/port.
///
/// # Notes on usage
/// Both `remote_ip` and `remote_port` are unset until the media address of the remote endpoint is
/// known, which happens after calling `transport.connect()` in `PlainTransport` and
/// `PipeTransport`, or via dynamic detection as it happens in `WebRtcTransport` (in which the
/// remote media address is detected by ICE means), or in `PlainTransport` (when using `comedia`
/// mode).
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(untagged)]
pub enum TransportTuple {
    /// Transport tuple with remote endpoint info.
    #[serde(rename_all = "camelCase")]
    WithRemote {
        /// Local IP address.
        local_ip: IpAddr,
        /// Local port.
        local_port: u16,
        /// Remote IP address.
        remote_ip: IpAddr,
        /// Remote port.
        remote_port: u16,
        /// Protocol
        protocol: Protocol,
    },
    /// Transport tuple without remote endpoint info.
    #[serde(rename_all = "camelCase")]
    LocalOnly {
        /// Local IP address.
        local_ip: IpAddr,
        /// Local port.
        local_port: u16,
        /// Protocol
        protocol: Protocol,
    },
}

impl TransportTuple {
    /// Local IP address.
    pub fn local_ip(&self) -> IpAddr {
        let (Self::WithRemote { local_ip, .. } | Self::LocalOnly { local_ip, .. }) = self;
        *local_ip
    }

    /// Local port.
    pub fn local_port(&self) -> u16 {
        let (Self::WithRemote { local_port, .. } | Self::LocalOnly { local_port, .. }) = self;
        *local_port
    }

    /// Protocol.
    pub fn protocol(&self) -> Protocol {
        let (Self::WithRemote { protocol, .. } | Self::LocalOnly { protocol, .. }) = self;
        *protocol
    }

    /// Remote IP address.
    pub fn remote_ip(&self) -> Option<IpAddr> {
        if let TransportTuple::WithRemote { remote_ip, .. } = self {
            Some(*remote_ip)
        } else {
            None
        }
    }

    /// Remote port.
    pub fn remote_port(&self) -> Option<u16> {
        if let TransportTuple::WithRemote { remote_port, .. } = self {
            Some(*remote_port)
        } else {
            None
        }
    }
}

/// DTLS state.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum DtlsState {
    /// DTLS procedures not yet initiated.
    New,
    /// DTLS connecting.
    Connecting,
    /// DTLS successfully connected (SRTP keys already extracted).
    Connected,
    /// DTLS connection failed.
    Failed,
    /// DTLS state when the `transport` has been closed.
    Closed,
}

/// SCTP state.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum SctpState {
    /// SCTP procedures not yet initiated.
    New,
    /// SCTP connecting.
    Connecting,
    /// SCTP successfully connected.
    Connected,
    /// SCTP connection failed.
    Failed,
    /// SCTP state when the transport has been closed.
    Closed,
}

/// DTLS role.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub enum DtlsRole {
    /// The DTLS role is determined based on the resolved ICE role (the `Controlled` role acts as
    /// DTLS client, the `Controlling` role acts as DTLS server).
    /// Since mediasoup is a ICE Lite implementation it always behaves as ICE `Controlled`.
    Auto,
    /// DTLS client role.
    Client,
    /// DTLS server role.
    Server,
}

impl Default for DtlsRole {
    fn default() -> Self {
        Self::Auto
    }
}

/// The hash function algorithm (as defined in the "Hash function Textual Names" registry initially
/// specified in [RFC 4572](https://tools.ietf.org/html/rfc4572#section-8) Section 8) and its
/// corresponding certificate fingerprint value.
#[derive(Copy, Clone, PartialOrd, PartialEq)]
pub enum DtlsFingerprint {
    /// sha-1
    Sha1 {
        /// Certificate fingerprint value.
        value: [u8; 20],
    },
    /// sha-224
    Sha224 {
        /// Certificate fingerprint value.
        value: [u8; 28],
    },
    /// sha-256
    Sha256 {
        /// Certificate fingerprint value.
        value: [u8; 32],
    },
    /// sha-384
    Sha384 {
        /// Certificate fingerprint value.
        value: [u8; 48],
    },
    /// sha-512
    Sha512 {
        /// Certificate fingerprint value.
        value: [u8; 64],
    },
}

impl fmt::Debug for DtlsFingerprint {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let name = match self {
            DtlsFingerprint::Sha1 { .. } => "Sha1",
            DtlsFingerprint::Sha224 { .. } => "Sha224",
            DtlsFingerprint::Sha256 { .. } => "Sha256",
            DtlsFingerprint::Sha384 { .. } => "Sha384",
            DtlsFingerprint::Sha512 { .. } => "Sha512",
        };
        f.debug_struct(name)
            .field("value", &self.value_string())
            .finish()
    }
}

impl Serialize for DtlsFingerprint {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let mut rtcp_feedback = serializer.serialize_struct("DtlsFingerprint", 2)?;
        rtcp_feedback.serialize_field("algorithm", self.algorithm_str())?;
        rtcp_feedback.serialize_field("value", &self.value_string())?;
        rtcp_feedback.end()
    }
}

impl<'de> Deserialize<'de> for DtlsFingerprint {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        #[derive(Deserialize)]
        #[serde(field_identifier, rename_all = "lowercase")]
        enum Field {
            Algorithm,
            Value,
        }

        struct DtlsFingerprintVisitor;

        impl<'de> Visitor<'de> for DtlsFingerprintVisitor {
            type Value = DtlsFingerprint;

            fn expecting(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
                formatter.write_str(
                    r#"DTLS fingerprint algorithm and value like {"algorithm": "sha-256", "value": "1B:EA:BF:33:B8:11:26:6D:91:AD:1B:A0:16:FD:5D:60:59:33:F7:46:A3:BA:99:2A:1D:04:99:A6:F2:C6:2D:43"}"#,
                )
            }

            fn visit_map<V>(self, mut map: V) -> Result<Self::Value, V::Error>
            where
                V: MapAccess<'de>,
            {
                fn parse_as_bytes(input: &str, output: &mut [u8]) -> Result<(), String> {
                    for (i, v) in output.iter_mut().enumerate() {
                        *v = u8::from_str_radix(&input[i * 3..(i * 3) + 2], 16).map_err(
                            |error| {
                                format!(
                                    "Failed to parse value {} as series of hex bytes: {}",
                                    input, error,
                                )
                            },
                        )?;
                    }

                    Ok(())
                }

                let mut algorithm = None::<Cow<'_, str>>;
                let mut value = None::<Cow<'_, str>>;
                while let Some(key) = map.next_key()? {
                    match key {
                        Field::Algorithm => {
                            if algorithm.is_some() {
                                return Err(de::Error::duplicate_field("algorithm"));
                            }
                            algorithm = Some(map.next_value()?);
                        }
                        Field::Value => {
                            if value.is_some() {
                                return Err(de::Error::duplicate_field("value"));
                            }
                            value = map.next_value()?;
                        }
                    }
                }
                let algorithm = algorithm.ok_or_else(|| de::Error::missing_field("algorithm"))?;
                let value = value.ok_or_else(|| de::Error::missing_field("value"))?;

                match algorithm.as_ref() {
                    "sha-1" => {
                        if value.len() == (20 * 3 - 1) {
                            let mut value_result = [0_u8; 20];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(de::Error::custom)?;

                            Ok(DtlsFingerprint::Sha1 {
                                value: value_result,
                            })
                        } else {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-1",
                            ))
                        }
                    }
                    "sha-224" => {
                        if value.len() == (28 * 3 - 1) {
                            let mut value_result = [0_u8; 28];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(de::Error::custom)?;

                            Ok(DtlsFingerprint::Sha224 {
                                value: value_result,
                            })
                        } else {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-224",
                            ))
                        }
                    }
                    "sha-256" => {
                        if value.len() == (32 * 3 - 1) {
                            let mut value_result = [0_u8; 32];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(de::Error::custom)?;

                            Ok(DtlsFingerprint::Sha256 {
                                value: value_result,
                            })
                        } else {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-256",
                            ))
                        }
                    }
                    "sha-384" => {
                        if value.len() == (48 * 3 - 1) {
                            let mut value_result = [0_u8; 48];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(de::Error::custom)?;

                            Ok(DtlsFingerprint::Sha384 {
                                value: value_result,
                            })
                        } else {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-384",
                            ))
                        }
                    }
                    "sha-512" => {
                        if value.len() == (64 * 3 - 1) {
                            let mut value_result = [0_u8; 64];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(de::Error::custom)?;

                            Ok(DtlsFingerprint::Sha512 {
                                value: value_result,
                            })
                        } else {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-512",
                            ))
                        }
                    }
                    algorithm => Err(de::Error::unknown_variant(
                        algorithm,
                        &["sha-1", "sha-224", "sha-256", "sha-384", "sha-512"],
                    )),
                }
            }
        }

        const FIELDS: &[&str] = &["algorithm", "value"];
        deserializer.deserialize_struct("DtlsFingerprint", FIELDS, DtlsFingerprintVisitor)
    }
}

impl DtlsFingerprint {
    fn value_string(&self) -> String {
        match self {
            DtlsFingerprint::Sha1 { value } => {
                format!(
                    "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                    value[0],
                    value[1],
                    value[2],
                    value[3],
                    value[4],
                    value[5],
                    value[6],
                    value[7],
                    value[8],
                    value[9],
                    value[10],
                    value[11],
                    value[12],
                    value[13],
                    value[14],
                    value[15],
                    value[16],
                    value[17],
                    value[18],
                    value[19],
                )
            }
            DtlsFingerprint::Sha224 { value } => {
                format!(
                    "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                    value[0],
                    value[1],
                    value[2],
                    value[3],
                    value[4],
                    value[5],
                    value[6],
                    value[7],
                    value[8],
                    value[9],
                    value[10],
                    value[11],
                    value[12],
                    value[13],
                    value[14],
                    value[15],
                    value[16],
                    value[17],
                    value[18],
                    value[19],
                    value[20],
                    value[21],
                    value[22],
                    value[23],
                    value[24],
                    value[25],
                    value[26],
                    value[27],
                )
            }
            DtlsFingerprint::Sha256 { value } => {
                format!(
                    "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}",
                    value[0],
                    value[1],
                    value[2],
                    value[3],
                    value[4],
                    value[5],
                    value[6],
                    value[7],
                    value[8],
                    value[9],
                    value[10],
                    value[11],
                    value[12],
                    value[13],
                    value[14],
                    value[15],
                    value[16],
                    value[17],
                    value[18],
                    value[19],
                    value[20],
                    value[21],
                    value[22],
                    value[23],
                    value[24],
                    value[25],
                    value[26],
                    value[27],
                    value[28],
                    value[29],
                    value[30],
                    value[31],
                )
            }
            DtlsFingerprint::Sha384 { value } => {
                format!(
                    "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}",
                    value[0],
                    value[1],
                    value[2],
                    value[3],
                    value[4],
                    value[5],
                    value[6],
                    value[7],
                    value[8],
                    value[9],
                    value[10],
                    value[11],
                    value[12],
                    value[13],
                    value[14],
                    value[15],
                    value[16],
                    value[17],
                    value[18],
                    value[19],
                    value[20],
                    value[21],
                    value[22],
                    value[23],
                    value[24],
                    value[25],
                    value[26],
                    value[27],
                    value[28],
                    value[29],
                    value[30],
                    value[31],
                    value[32],
                    value[33],
                    value[34],
                    value[35],
                    value[36],
                    value[37],
                    value[38],
                    value[39],
                    value[40],
                    value[41],
                    value[42],
                    value[43],
                    value[44],
                    value[45],
                    value[46],
                    value[47],
                )
            }
            DtlsFingerprint::Sha512 { value } => {
                format!(
                    "{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:{:02X}:\
                    {:02X}:{:02X}:{:02X}:{:02X}",
                    value[0],
                    value[1],
                    value[2],
                    value[3],
                    value[4],
                    value[5],
                    value[6],
                    value[7],
                    value[8],
                    value[9],
                    value[10],
                    value[11],
                    value[12],
                    value[13],
                    value[14],
                    value[15],
                    value[16],
                    value[17],
                    value[18],
                    value[19],
                    value[20],
                    value[21],
                    value[22],
                    value[23],
                    value[24],
                    value[25],
                    value[26],
                    value[27],
                    value[28],
                    value[29],
                    value[30],
                    value[31],
                    value[32],
                    value[33],
                    value[34],
                    value[35],
                    value[36],
                    value[37],
                    value[38],
                    value[39],
                    value[40],
                    value[41],
                    value[42],
                    value[43],
                    value[44],
                    value[45],
                    value[46],
                    value[47],
                    value[48],
                    value[49],
                    value[50],
                    value[51],
                    value[52],
                    value[53],
                    value[54],
                    value[55],
                    value[56],
                    value[57],
                    value[58],
                    value[59],
                    value[60],
                    value[61],
                    value[62],
                    value[63],
                )
            }
        }
    }

    fn algorithm_str(&self) -> &'static str {
        match self {
            DtlsFingerprint::Sha1 { .. } => "sha-1",
            DtlsFingerprint::Sha224 { .. } => "sha-224",
            DtlsFingerprint::Sha256 { .. } => "sha-256",
            DtlsFingerprint::Sha384 { .. } => "sha-384",
            DtlsFingerprint::Sha512 { .. } => "sha-512",
        }
    }
}

/// DTLS parameters.
#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
pub struct DtlsParameters {
    /// DTLS role.
    pub role: DtlsRole,
    /// DTLS fingerprints.
    pub fingerprints: Vec<DtlsFingerprint>,
}

/// Trace event direction
#[derive(Debug, Copy, Clone, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum TraceEventDirection {
    /// In
    In,
    /// Out
    Out,
}

/// Container used for sending/receiving messages using `DirectTransport` data producers and data
/// consumers.
#[derive(Debug, Clone)]
pub enum WebRtcMessage<'a> {
    /// String
    String(String),
    /// Binary
    Binary(Cow<'a, [u8]>),
    /// EmptyString
    EmptyString,
    /// EmptyBinary
    EmptyBinary,
}

impl<'a> WebRtcMessage<'a> {
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

    pub(crate) fn new(ppid: u32, payload: Cow<'a, [u8]>) -> Result<Self, u32> {
        match ppid {
            51 => Ok(WebRtcMessage::String(
                String::from_utf8(payload.to_vec()).unwrap(),
            )),
            53 => Ok(WebRtcMessage::Binary(payload)),
            56 => Ok(WebRtcMessage::EmptyString),
            57 => Ok(WebRtcMessage::EmptyBinary),
            ppid => Err(ppid),
        }
    }

    pub(crate) fn into_ppid_and_payload(self) -> (u32, Cow<'a, [u8]>) {
        match self {
            WebRtcMessage::String(string) => (51_u32, Cow::from(string.into_bytes())),
            WebRtcMessage::Binary(binary) => (53_u32, binary),
            WebRtcMessage::EmptyString => (56_u32, Cow::from(b" ".as_ref())),
            WebRtcMessage::EmptyBinary => (57_u32, Cow::from(vec![0_u8])),
        }
    }
}

/// RTP packet info in trace event.
#[derive(Debug, Clone, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RtpPacketTraceInfo {
    /// RTP payload type.
    pub payload_type: u8,
    /// Sequence number.
    pub sequence_number: u16,
    /// Timestamp.
    pub timestamp: u32,
    /// Whether packet has marker or not.
    pub marker: bool,
    /// RTP stream SSRC.
    pub ssrc: u32,
    /// Whether packet contains a key frame.
    pub is_key_frame: bool,
    /// Packet size.
    pub size: usize,
    /// Payload size.
    pub payload_size: usize,
    /// The spatial layer index (from 0 to N).
    pub spatial_layer: u8,
    /// The temporal layer index (from 0 to N).
    pub temporal_layer: u8,
    /// The MID RTP extension value as defined in the BUNDLE specification
    pub mid: Option<String>,
    /// RTP stream RID value.
    pub rid: Option<String>,
    /// RTP stream RRID value.
    pub rrid: Option<String>,
    /// Transport-wide sequence number.
    pub wide_sequence_number: Option<u16>,
    /// Whether this is an RTX packet.
    #[serde(default)]
    pub is_rtx: bool,
}

/// SSRC info in trace event.
#[derive(Debug, Copy, Clone, Deserialize, Serialize)]
pub struct SsrcTraceInfo {
    /// RTP stream SSRC.
    ssrc: u32,
}

/// Bandwidth estimation type.
#[derive(Debug, Copy, Clone, Deserialize, Serialize)]
pub enum BweType {
    /// Transport-wide Congestion Control.
    #[serde(rename = "transport-cc")]
    TransportCc,
    /// Receiver Estimated Maximum Bitrate.
    #[serde(rename = "remb")]
    Remb,
}

/// BWE info in trace event.
#[derive(Debug, Copy, Clone, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct BweTraceInfo {
    /// Bandwidth estimation type.
    r#type: BweType,
    /// Desired bitrate
    desired_bitrate: u32,
    /// Effective desired bitrate.
    effective_desired_bitrate: u32,
    /// Min bitrate.
    min_bitrate: u32,
    /// Max bitrate.
    max_bitrate: u32,
    /// Start bitrate.
    start_bitrate: u32,
    /// Max padding bitrate.
    max_padding_bitrate: u32,
    /// Available bitrate.
    available_bitrate: u32,
}
