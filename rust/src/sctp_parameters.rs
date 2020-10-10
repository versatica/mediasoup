use serde::{Deserialize, Serialize};

pub struct SctpCapabilities {
    num_streams: NumSctpStreams,
}

/// Both OS and MIS are part of the SCTP INIT+ACK handshake. OS refers to the initial number of
/// outgoing SCTP streams that the server side transport creates (to be used by DataConsumers),
/// while MIS refers to the maximum number of incoming SCTP streams that the server side transport
/// can receive (to be used by DataProducers). So, if the server side transport will just be used to
/// create data producers (but no data consumers), OS can be low (~1). However, if data consumers
/// are desired on the server side transport, OS must have a proper value and such a proper value
/// depends on whether the remote endpoint supports  SCTP_ADD_STREAMS extension or not.
///
/// libwebrtc (Chrome, Safari, etc) does not enable SCTP_ADD_STREAMS so, if data consumers are
/// required,  OS should be 1024 (the maximum number of DataChannels that libwebrtc enables).
///
/// Firefox does enable SCTP_ADD_STREAMS so, if data consumers are required, OS can be lower (16 for
/// instance). The mediasoup transport will allocate and announce more outgoing SCTP streams when
/// needed.
///
/// mediasoup-client provides specific per browser/version OS and MIS values via the
/// device.sctpCapabilities getter.
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

#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
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

// TODO: Enum to protect documented invariants
/// SCTP stream parameters describe the reliability of a certain SCTP stream.
/// If ordered is true then max_packet_life_time and max_retransmits must be false.
/// If ordered if false, only one of max_packet_life_time or max_retransmits can be true.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct SctpStreamParameters {
    /// SCTP stream id.
    stream_id: u16,
    /// Whether data messages must be received in order. If true the messages will be sent reliably.
    /// Default true.
    ordered: bool,
    /// When ordered is false indicates the time (in milliseconds) after which a SCTP packet will
    /// stop being retransmitted.
    #[serde(skip_serializing_if = "Option::is_none")]
    max_packet_life_time: Option<u16>,
    /// When ordered is false indicates the maximum number of times a packet will be retransmitted.
    #[serde(skip_serializing_if = "Option::is_none")]
    max_retransmits: Option<u16>,
}

impl SctpStreamParameters {
    /// Messages will be sent reliably in order.
    pub fn new_ordered(stream_id: u16) -> Self {
        Self {
            stream_id,
            ordered: true,
            max_packet_life_time: None,
            max_retransmits: None,
        }
    }

    /// Messages will be sent unreliably with time (in milliseconds) after which a SCTP packet will
    /// stop being retransmitted.
    pub fn new_unordered_with_life_time(stream_id: u16, max_packet_life_time: u16) -> Self {
        Self {
            stream_id,
            ordered: false,
            max_packet_life_time: Some(max_packet_life_time),
            max_retransmits: None,
        }
    }

    /// Messages will be sent unreliably with a limited number of retransmission attempts.
    pub fn new_unordered_with_retransmits(stream_id: u16, max_retransmits: u16) -> Self {
        Self {
            stream_id,
            ordered: false,
            max_packet_life_time: None,
            max_retransmits: Some(max_retransmits),
        }
    }
}
