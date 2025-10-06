//! SCTP parameters.

use crate::fbs::{FromFbs, ToFbs};
use mediasoup_sys::fbs::sctp_parameters;
use mediasoup_types::sctp_parameters::*;

impl ToFbs for NumSctpStreams {
    type FbsType = sctp_parameters::NumSctpStreams;

    fn to_fbs(&self) -> Self::FbsType {
        sctp_parameters::NumSctpStreams {
            os: self.os,
            mis: self.mis,
        }
    }
}

impl FromFbs for SctpParameters {
    type FbsType = sctp_parameters::SctpParameters;

    fn from_fbs(parameters: &Self::FbsType) -> Self {
        Self {
            port: parameters.port,
            os: parameters.os,
            mis: parameters.mis,
            max_message_size: parameters.max_message_size,
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
