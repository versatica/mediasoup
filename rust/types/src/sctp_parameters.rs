//! Collection of SCTP-related data structures that are used to specify SCTP association parameters.

use serde::{Deserialize, Serialize};

/// Parameters of the SCTP association.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct SctpParameters {
    pub port: u16,
    /// Maximum allowed size for SCTP send messages.
    pub max_send_message_size: u32,
    /// Maximum allowed size for SCTP receive messages.
    pub max_receive_message_size: u32,
    /// Size of the SCTP send buffer.
    pub send_buffer_size: u32,
    /// Per-stream send queue limit.
    pub per_stream_send_queue_limit: u32,
    /// Maximum receiver window buffer size.
    pub max_receiver_window_buffer_size: u32,
    /// Whether this is a DataChannel SCTP association.
    pub is_data_channel: bool,

    // TODO: SCTP: For backwards compatibility. Remove them in the future.
    #[serde(rename = "OS")]
    pub os: u16,
    #[serde(rename = "MIS")]
    pub mis: u16,
    pub max_message_size: u32,
}

/// SCTP negotiated capabilities (only available once SCTP association is connected).
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct SctpNegotiatedCapabilities {
    pub negotiated_max_outbound_streams: u16,
    pub negotiated_max_inbound_streams: u16,
}

/// SCTP stream parameters describe the reliability of a certain SCTP stream.
///
/// If ordered is true then `max_packet_life_time` and `max_retransmits` must be `false`.
/// If ordered if false, only one of `max_packet_life_time` or max_retransmits can be `true`.
#[derive(Debug, Copy, Clone, Ord, PartialOrd, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct SctpStreamParameters {
    /// SCTP stream id.
    pub stream_id: u16,
    /// Whether data messages must be received in order. If `true` the messages will be sent
    /// reliably.
    /// Default true.
    pub ordered: bool,
    /// When `ordered` is `false` indicates the time (in milliseconds) after which a SCTP packet
    /// will stop being retransmitted.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub max_packet_life_time: Option<u16>,
    /// When `ordered` is `false` indicates the maximum number of times a packet will be
    /// retransmitted.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub max_retransmits: Option<u16>,
}

impl SctpStreamParameters {
    /// SCTP stream id.
    #[must_use]
    pub fn stream_id(&self) -> u16 {
        self.stream_id
    }

    /// Whether data messages must be received in order. If `true` the messages will be sent
    /// reliably.
    #[must_use]
    pub fn ordered(&self) -> bool {
        self.ordered
    }

    /// When `ordered` is `false` indicates the time (in milliseconds) after which a SCTP packet
    /// will stop being retransmitted.
    #[must_use]
    pub fn max_packet_life_time(&self) -> Option<u16> {
        self.max_packet_life_time
    }

    /// When `ordered` is `false` indicates the maximum number of times a packet will be
    /// retransmitted.
    #[must_use]
    pub fn max_retransmits(&self) -> Option<u16> {
        self.max_retransmits
    }
}

impl SctpStreamParameters {
    /// Messages will be sent reliably in order.
    #[must_use]
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
    #[must_use]
    pub fn new_unordered_with_life_time(stream_id: u16, max_packet_life_time: u16) -> Self {
        Self {
            stream_id,
            ordered: false,
            max_packet_life_time: Some(max_packet_life_time),
            max_retransmits: None,
        }
    }

    /// Messages will be sent unreliably with a limited number of retransmission attempts.
    #[must_use]
    pub fn new_unordered_with_retransmits(stream_id: u16, max_retransmits: u16) -> Self {
        Self {
            stream_id,
            ordered: false,
            max_packet_life_time: None,
            max_retransmits: Some(max_retransmits),
        }
    }
}
