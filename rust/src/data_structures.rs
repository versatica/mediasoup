use bytes::Bytes;
use serde::de::{MapAccess, Visitor};
use serde::ser::SerializeStruct;
use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
use std::any::Any;
use std::borrow::Cow;
use std::fmt;
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

#[derive(Debug, Copy, Clone, PartialOrd, PartialEq)]
pub enum DtlsFingerprint {
    Sha1 { value: [u8; 20] },
    Sha224 { value: [u8; 28] },
    Sha256 { value: [u8; 32] },
    Sha384 { value: [u8; 48] },
    Sha512 { value: [u8; 64] },
}

impl Serialize for DtlsFingerprint {
    fn serialize<S>(&self, serializer: S) -> Result<<S as Serializer>::Ok, <S as Serializer>::Error>
    where
        S: Serializer,
    {
        let mut rtcp_feedback = serializer.serialize_struct("DtlsFingerprint", 2)?;
        match self {
            DtlsFingerprint::Sha1 { value } => {
                let value = format!(
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
                );
                rtcp_feedback.serialize_field("algorithm", "sha-1")?;
                rtcp_feedback.serialize_field("value", &value)?;
            }
            DtlsFingerprint::Sha224 { value } => {
                let value = format!(
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
                );
                rtcp_feedback.serialize_field("algorithm", "sha-224")?;
                rtcp_feedback.serialize_field("value", &value)?;
            }
            DtlsFingerprint::Sha256 { value } => {
                let value = format!(
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
                );
                rtcp_feedback.serialize_field("algorithm", "sha-256")?;
                rtcp_feedback.serialize_field("value", &value)?;
            }
            DtlsFingerprint::Sha384 { value } => {
                let value = format!(
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
                );
                rtcp_feedback.serialize_field("algorithm", "sha-384")?;
                rtcp_feedback.serialize_field("value", &value)?;
            }
            DtlsFingerprint::Sha512 { value } => {
                let value = format!(
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
                );
                rtcp_feedback.serialize_field("algorithm", "sha-512")?;
                rtcp_feedback.serialize_field("value", &value)?;
            }
        }
        rtcp_feedback.end()
    }
}

impl<'de> Deserialize<'de> for DtlsFingerprint {
    fn deserialize<D>(deserializer: D) -> Result<Self, <D as Deserializer<'de>>::Error>
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

            fn expecting(&self, formatter: &mut fmt::Formatter) -> fmt::Result {
                formatter.write_str(
                    r#"DTLS fingerprint algorithm and value like {"algorithm": "sha-256", "value": "1B:EA:BF:33:B8:11:26:6D:91:AD:1B:A0:16:FD:5D:60:59:33:F7:46:A3:BA:99:2A:1D:04:99:A6:F2:C6:2D:43"}"#,
                )
            }

            fn visit_map<V>(self, mut map: V) -> Result<Self::Value, V::Error>
            where
                V: MapAccess<'de>,
            {
                let mut algorithm = None::<Cow<str>>;
                let mut value = None::<Cow<str>>;
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

                match algorithm.as_ref() {
                    "sha-1" => {
                        if value.len() != (20 * 3 - 1) {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-1",
                            ))
                        } else {
                            let mut value_result = [0u8; 20];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(|error| de::Error::custom(error))?;

                            Ok(DtlsFingerprint::Sha1 {
                                value: value_result,
                            })
                        }
                    }
                    "sha-224" => {
                        if value.len() != (28 * 3 - 1) {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-224",
                            ))
                        } else {
                            let mut value_result = [0u8; 28];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(|error| de::Error::custom(error))?;

                            Ok(DtlsFingerprint::Sha224 {
                                value: value_result,
                            })
                        }
                    }
                    "sha-256" => {
                        if value.len() != (32 * 3 - 1) {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-256",
                            ))
                        } else {
                            let mut value_result = [0u8; 32];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(|error| de::Error::custom(error))?;

                            Ok(DtlsFingerprint::Sha256 {
                                value: value_result,
                            })
                        }
                    }
                    "sha-384" => {
                        if value.len() != (48 * 3 - 1) {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-384",
                            ))
                        } else {
                            let mut value_result = [0u8; 48];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(|error| de::Error::custom(error))?;

                            Ok(DtlsFingerprint::Sha384 {
                                value: value_result,
                            })
                        }
                    }
                    "sha-512" => {
                        if value.len() != (64 * 3 - 1) {
                            Err(de::Error::custom(
                                "Value doesn't have correct length for SHA-512",
                            ))
                        } else {
                            let mut value_result = [0u8; 64];
                            parse_as_bytes(value.as_ref(), &mut value_result)
                                .map_err(|error| de::Error::custom(error))?;

                            Ok(DtlsFingerprint::Sha512 {
                                value: value_result,
                            })
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn dtls_fingerprint() {
        {
            let dtls_fingerprints = &[
                r#"{"algorithm":"sha-1","value":"0D:88:5B:EF:B9:86:F9:66:67:75:7A:C1:7A:78:34:E4:88:DC:44:67"}"#,
                r#"{"algorithm":"sha-224","value":"6E:0C:C7:23:DF:36:E1:C7:46:AB:D7:B1:CE:DD:97:C3:C1:17:25:D6:26:0A:8A:B4:50:F1:3E:BC"}"#,
                r#"{"algorithm":"sha-256","value":"7A:27:46:F0:7B:09:28:F0:10:E2:EC:84:60:B5:87:9A:D9:C8:8B:F3:6C:C5:5D:C3:F3:BA:2C:5B:4F:8A:3A:E3"}"#,
                r#"{"algorithm":"sha-384","value":"D0:B7:F7:3C:71:9F:F4:A1:48:E1:9B:13:25:59:A4:7D:06:BF:E1:1B:DC:0B:8A:8E:45:09:01:22:7E:81:68:EC:DD:B8:DD:CA:1F:F3:F2:E8:15:A5:3C:23:CF:F7:B6:38"}"#,
                r#"{"algorithm":"sha-512","value":"36:8B:9B:CA:2B:01:2B:33:FD:06:95:F2:CC:28:56:69:5B:DD:38:5E:88:32:9A:72:F7:B1:5D:87:9E:64:97:0B:66:A1:C7:6C:BE:4D:CD:83:90:04:AE:20:6C:6D:5F:F0:BD:4C:D9:DD:6E:8A:19:C1:C9:F6:C2:46:C8:08:94:39"}"#,
            ];

            for dtls_fingerprint_str in dtls_fingerprints {
                let dtls_fingerprint =
                    serde_json::from_str::<DtlsFingerprint>(dtls_fingerprint_str).unwrap();
                assert_eq!(
                    dtls_fingerprint_str,
                    &serde_json::to_string(&dtls_fingerprint).unwrap()
                );
            }
        }

        {
            let bad_dtls_fingerprints = &[
                r#"{"algorithm":"sha-1","value":"0D:88:5B:EF:B9:86:F9:66:67::44:67"}"#,
                r#"{"algorithm":"sha-200","value":"6E:0C:C7:23:DF:36:E1:C7:46:AB:D7:B1:CE:DD:97:C3:C1:17:25:D6:26:0A:8A:B4:50:F1:3E:BC"}"#,
            ];

            for dtls_fingerprint_str in bad_dtls_fingerprints {
                assert!(serde_json::from_str::<DtlsFingerprint>(dtls_fingerprint_str).is_err());
            }
        }
    }
}
