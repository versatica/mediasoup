//! Miscellaneous data structures.

use mediasoup_sys::fbs::{
    common, producer, rtp_packet, sctp_association, transport, web_rtc_transport,
};
pub use mediasoup_types::data_structures::*;

use crate::fbs::{FromFbs, ToFbs};

impl ToFbs for ListenInfo {
    type FbsType = transport::ListenInfo;

    fn to_fbs(&self) -> Self::FbsType {
        transport::ListenInfo {
            protocol: match self.protocol {
                Protocol::Tcp => transport::Protocol::Tcp,
                Protocol::Udp => transport::Protocol::Udp,
            },
            ip: self.ip.to_string(),
            announced_address: self
                .announced_address
                .as_ref()
                .map(|address| address.to_string()),
            expose_internal_ip: self.expose_internal_ip,
            port: self.port.unwrap_or(0),
            port_range: match &self.port_range {
                Some(port_range) => Box::new(transport::PortRange {
                    min: *port_range.start(),
                    max: *port_range.end(),
                }),
                None => Box::new(transport::PortRange { min: 0, max: 0 }),
            },
            flags: Box::new(self.flags.unwrap_or_default().to_fbs()),
            send_buffer_size: self.send_buffer_size.unwrap_or(0),
            recv_buffer_size: self.recv_buffer_size.unwrap_or(0),
        }
    }
}

impl ToFbs for SocketFlags {
    type FbsType = transport::SocketFlags;

    fn to_fbs(&self) -> Self::FbsType {
        transport::SocketFlags {
            ipv6_only: self.ipv6_only,
            udp_reuse_port: self.udp_reuse_port,
        }
    }
}

impl ToFbs for DtlsRole {
    type FbsType = web_rtc_transport::DtlsRole;

    fn to_fbs(&self) -> Self::FbsType {
        match self {
            DtlsRole::Auto => web_rtc_transport::DtlsRole::Auto,
            DtlsRole::Client => web_rtc_transport::DtlsRole::Client,
            DtlsRole::Server => web_rtc_transport::DtlsRole::Server,
        }
    }
}

impl FromFbs for DtlsRole {
    type FbsType = web_rtc_transport::DtlsRole;

    fn from_fbs(role: &Self::FbsType) -> Self {
        match role {
            web_rtc_transport::DtlsRole::Auto => DtlsRole::Auto,
            web_rtc_transport::DtlsRole::Client => DtlsRole::Client,
            web_rtc_transport::DtlsRole::Server => DtlsRole::Server,
        }
    }
}

impl FromFbs for DtlsState {
    type FbsType = web_rtc_transport::DtlsState;

    fn from_fbs(state: &Self::FbsType) -> Self {
        match state {
            web_rtc_transport::DtlsState::New => DtlsState::New,
            web_rtc_transport::DtlsState::Connecting => DtlsState::Connecting,
            web_rtc_transport::DtlsState::Connected => DtlsState::Connected,
            web_rtc_transport::DtlsState::Failed => DtlsState::Failed,
            web_rtc_transport::DtlsState::Closed => DtlsState::Closed,
        }
    }
}

impl ToFbs for DtlsParameters {
    type FbsType = web_rtc_transport::DtlsParameters;

    fn to_fbs(&self) -> Self::FbsType {
        web_rtc_transport::DtlsParameters {
            role: self.role.to_fbs(),
            fingerprints: self
                .fingerprints
                .iter()
                .map(DtlsFingerprint::to_fbs)
                .collect(),
        }
    }
}

impl FromFbs for DtlsParameters {
    type FbsType = web_rtc_transport::DtlsParameters;

    fn from_fbs(parameters: &Self::FbsType) -> Self {
        DtlsParameters {
            role: DtlsRole::from_fbs(&parameters.role),
            fingerprints: parameters
                .fingerprints
                .iter()
                .map(|fingerprint| DtlsFingerprint::from_fbs(&fingerprint.clone()))
                .collect(),
        }
    }
}

impl ToFbs for DtlsFingerprint {
    type FbsType = web_rtc_transport::Fingerprint;

    fn to_fbs(&self) -> Self::FbsType {
        match self {
            DtlsFingerprint::Sha1 { .. } => web_rtc_transport::Fingerprint {
                algorithm: web_rtc_transport::FingerprintAlgorithm::Sha1,
                value: self.value_string(),
            },
            DtlsFingerprint::Sha224 { .. } => web_rtc_transport::Fingerprint {
                algorithm: web_rtc_transport::FingerprintAlgorithm::Sha224,
                value: self.value_string(),
            },
            DtlsFingerprint::Sha256 { .. } => web_rtc_transport::Fingerprint {
                algorithm: web_rtc_transport::FingerprintAlgorithm::Sha256,
                value: self.value_string(),
            },
            DtlsFingerprint::Sha384 { .. } => web_rtc_transport::Fingerprint {
                algorithm: web_rtc_transport::FingerprintAlgorithm::Sha384,
                value: self.value_string(),
            },
            DtlsFingerprint::Sha512 { .. } => web_rtc_transport::Fingerprint {
                algorithm: web_rtc_transport::FingerprintAlgorithm::Sha512,
                value: self.value_string(),
            },
        }
    }
}

/// Parses a series of hex bytes into a byte array.
fn hex_as_bytes<const N: usize>(input: &str) -> [u8; N] {
    let mut output = [0_u8; N];
    for (i, o) in input.split(':').zip(&mut output.iter_mut()) {
        *o = u8::from_str_radix(i, 16).unwrap_or_else(|error| {
            panic!("Failed to parse value {i} as series of hex bytes: {error}")
        });
    }

    output
}

impl FromFbs for DtlsFingerprint {
    type FbsType = web_rtc_transport::Fingerprint;

    fn from_fbs(fingerprint: &Self::FbsType) -> Self {
        match fingerprint.algorithm {
            web_rtc_transport::FingerprintAlgorithm::Sha1 => {
                let value_result = hex_as_bytes::<20>(fingerprint.value.as_str());

                DtlsFingerprint::Sha1 {
                    value: value_result,
                }
            }
            web_rtc_transport::FingerprintAlgorithm::Sha224 => {
                let value_result = hex_as_bytes::<28>(fingerprint.value.as_str());

                DtlsFingerprint::Sha224 {
                    value: value_result,
                }
            }
            web_rtc_transport::FingerprintAlgorithm::Sha256 => {
                let value_result = hex_as_bytes::<32>(fingerprint.value.as_str());

                DtlsFingerprint::Sha256 {
                    value: value_result,
                }
            }
            web_rtc_transport::FingerprintAlgorithm::Sha384 => {
                let value_result = hex_as_bytes::<48>(fingerprint.value.as_str());

                DtlsFingerprint::Sha384 {
                    value: value_result,
                }
            }
            web_rtc_transport::FingerprintAlgorithm::Sha512 => {
                let value_result = hex_as_bytes::<64>(fingerprint.value.as_str());

                DtlsFingerprint::Sha512 {
                    value: value_result,
                }
            }
        }
    }
}

impl FromFbs for IceRole {
    type FbsType = web_rtc_transport::IceRole;

    fn from_fbs(role: &Self::FbsType) -> Self {
        match role {
            web_rtc_transport::IceRole::Controlled => IceRole::Controlled,
            web_rtc_transport::IceRole::Controlling => IceRole::Controlling,
        }
    }
}

impl FromFbs for IceParameters {
    type FbsType = web_rtc_transport::IceParameters;

    fn from_fbs(parameters: &Self::FbsType) -> Self {
        Self {
            username_fragment: parameters.username_fragment.to_string(),
            password: parameters.password.to_string(),
            ice_lite: Some(parameters.ice_lite),
        }
    }
}

impl FromFbs for SctpState {
    type FbsType = sctp_association::SctpState;

    fn from_fbs(state: &Self::FbsType) -> Self {
        match state {
            sctp_association::SctpState::New => Self::New,
            sctp_association::SctpState::Connecting => Self::Connecting,
            sctp_association::SctpState::Connected => Self::Connected,
            sctp_association::SctpState::Failed => Self::Failed,
            sctp_association::SctpState::Closed => Self::Closed,
        }
    }
}

impl FromFbs for IceCandidateType {
    type FbsType = web_rtc_transport::IceCandidateType;

    fn from_fbs(candidate_type: &Self::FbsType) -> Self {
        match candidate_type {
            web_rtc_transport::IceCandidateType::Host => IceCandidateType::Host,
        }
    }
}

impl FromFbs for IceCandidate {
    type FbsType = web_rtc_transport::IceCandidate;

    fn from_fbs(candidate: &Self::FbsType) -> Self {
        Self {
            foundation: candidate.foundation.clone(),
            priority: candidate.priority,
            address: candidate.address.clone(),
            protocol: Protocol::from_fbs(&candidate.protocol),
            port: candidate.port,
            r#type: IceCandidateType::from_fbs(&candidate.type_),
            tcp_type: FromFbs::from_fbs(&candidate.tcp_type),
        }
    }
}

impl FromFbs for IceCandidateTcpType {
    type FbsType = web_rtc_transport::IceCandidateTcpType;

    fn from_fbs(candidate_type: &Self::FbsType) -> Self {
        match candidate_type {
            web_rtc_transport::IceCandidateTcpType::Passive => IceCandidateTcpType::Passive,
        }
    }
}

impl FromFbs for IceState {
    type FbsType = web_rtc_transport::IceState;

    fn from_fbs(state: &Self::FbsType) -> Self {
        match state {
            web_rtc_transport::IceState::New => IceState::New,
            web_rtc_transport::IceState::Connected => IceState::Connected,
            web_rtc_transport::IceState::Completed => IceState::Completed,
            web_rtc_transport::IceState::Disconnected => IceState::Disconnected,
        }
    }
}

impl FromFbs for Protocol {
    type FbsType = transport::Protocol;

    fn from_fbs(protocol: &Self::FbsType) -> Self {
        match protocol {
            transport::Protocol::Tcp => Protocol::Tcp,
            transport::Protocol::Udp => Protocol::Udp,
        }
    }
}

impl FromFbs for TransportTuple {
    type FbsType = transport::Tuple;

    fn from_fbs(tuple: &Self::FbsType) -> Self {
        match &tuple.remote_ip {
            Some(_remote_ip) => TransportTuple::WithRemote {
                local_address: tuple
                    .local_address
                    .parse()
                    .expect("Error parsing local address"),
                local_port: tuple.local_port,
                remote_ip: tuple
                    .remote_ip
                    .as_ref()
                    .unwrap()
                    .parse()
                    .expect("Error parsing remote IP address"),
                remote_port: tuple.remote_port,
                protocol: Protocol::from_fbs(&tuple.protocol),
            },
            None => TransportTuple::LocalOnly {
                local_address: tuple
                    .local_address
                    .parse()
                    .expect("Error parsing local address"),
                local_port: tuple.local_port,
                protocol: Protocol::from_fbs(&tuple.protocol),
            },
        }
    }
}

impl FromFbs for TraceEventDirection {
    type FbsType = common::TraceDirection;

    fn from_fbs(event_type: &Self::FbsType) -> Self {
        match event_type {
            common::TraceDirection::DirectionIn => TraceEventDirection::In,
            common::TraceDirection::DirectionOut => TraceEventDirection::Out,
        }
    }
}

impl FromFbs for SrTraceInfo {
    type FbsType = producer::SrTraceInfo;

    fn from_fbs(info: &Self::FbsType) -> Self {
        Self {
            ssrc: info.ssrc,
            ntp_sec: info.ntp_sec,
            ntp_frac: info.ntp_frac,
            rtp_ts: info.rtp_ts,
            packet_count: info.packet_count,
            octet_count: info.octet_count,
        }
    }
}

impl FromFbs for BweType {
    type FbsType = transport::BweType;

    fn from_fbs(info: &Self::FbsType) -> Self {
        match info {
            transport::BweType::TransportCc => BweType::TransportCc,
            transport::BweType::Remb => BweType::Remb,
        }
    }
}

impl FromFbs for BweTraceInfo {
    type FbsType = transport::BweTraceInfo;

    fn from_fbs(info: &Self::FbsType) -> Self {
        Self {
            r#type: BweType::from_fbs(&info.bwe_type),
            desired_bitrate: info.desired_bitrate,
            effective_desired_bitrate: info.effective_desired_bitrate,
            min_bitrate: info.min_bitrate,
            max_bitrate: info.max_bitrate,
            start_bitrate: info.start_bitrate,
            max_padding_bitrate: info.max_padding_bitrate,
            available_bitrate: info.available_bitrate,
        }
    }
}

impl FromFbs for RtpPacketTraceInfo {
    type FbsType = rtp_packet::Dump;

    fn from_fbs(rtp_packet: &Self::FbsType) -> Self {
        Self {
            payload_type: rtp_packet.payload_type,
            sequence_number: rtp_packet.sequence_number,
            timestamp: rtp_packet.timestamp,
            marker: rtp_packet.marker,
            ssrc: rtp_packet.ssrc,
            is_key_frame: rtp_packet.is_key_frame,
            size: rtp_packet.size,
            payload_size: rtp_packet.payload_size,
            spatial_layer: rtp_packet.spatial_layer,
            temporal_layer: rtp_packet.temporal_layer,
            mid: rtp_packet.mid.clone(),
            rid: rtp_packet.rid.clone(),
            rrid: rtp_packet.rrid.clone(),
            wide_sequence_number: rtp_packet.wide_sequence_number,
            is_rtx: false,
        }
    }
}
