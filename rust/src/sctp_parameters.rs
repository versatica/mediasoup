//! SCTP parameters.

use crate::fbs::{FromFbs, ToFbs};
use mediasoup_sys::fbs::{sctp_association, sctp_parameters};
use mediasoup_types::sctp_parameters::*;

impl FromFbs for SctpParameters {
    type FbsType = sctp_parameters::SctpParameters;

    fn from_fbs(parameters: &Self::FbsType) -> Self {
        Self {
            port: parameters.port,
            max_send_message_size: parameters.max_send_message_size,
            max_receive_message_size: parameters.max_receive_message_size,
            send_buffer_size: parameters.send_buffer_size,
            per_stream_send_queue_limit: parameters.per_stream_send_queue_limit,
            max_receiver_window_buffer_size: parameters.max_receiver_window_buffer_size,
            is_data_channel: parameters.is_data_channel,

            // TODO: SCTP: For backwards compatibility. Remove them in the future.
            os: 65535,
            mis: 65535,
            max_message_size: parameters.max_receive_message_size,
        }
    }
}

impl FromFbs for SctpNegotiatedCapabilities {
    type FbsType = sctp_association::SctpNegotiatedCapabilities;

    fn from_fbs(negotiated_capabilities: &Self::FbsType) -> Self {
        SctpNegotiatedCapabilities {
            negotiated_max_outbound_streams: negotiated_capabilities
                .negotiated_max_outbound_streams,
            negotiated_max_inbound_streams: negotiated_capabilities.negotiated_max_inbound_streams,
        }
    }
}

impl FromFbs for SctpStreamParameters {
    type FbsType = sctp_parameters::SctpStreamParameters;

    fn from_fbs(stream_parameters: &Self::FbsType) -> Self {
        Self {
            stream_id: stream_parameters.stream_id,
            ordered: stream_parameters.ordered.unwrap_or(false),
            max_packet_life_time: stream_parameters.max_packet_life_time,
            max_retransmits: stream_parameters.max_retransmits,
        }
    }
}

impl ToFbs for SctpStreamParameters {
    type FbsType = sctp_parameters::SctpStreamParameters;

    fn to_fbs(&self) -> Self::FbsType {
        sctp_parameters::SctpStreamParameters {
            stream_id: self.stream_id,
            ordered: Some(self.ordered),
            max_packet_life_time: self.max_packet_life_time,
            max_retransmits: self.max_retransmits,
        }
    }
}
