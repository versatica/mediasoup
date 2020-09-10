use crate::data_structures::AppData;
use crate::rtp_parameters::{MediaKind, RtpParameters};
use crate::uuid_based_wrapper_type;

uuid_based_wrapper_type!(ProducerId);

#[derive(Debug)]
#[non_exhaustive]
pub struct ProducerOptions {
    /// Producer id, should most likely not be specified explicitly, specified by plain transport
    #[doc(hidden)]
    pub id: Option<ProducerId>,
    pub kind: MediaKind,
    // TODO: Docs have distinction between RtpSendParameters and RtpReceiveParameters
    pub rtp_parameters: RtpParameters,
    pub paused: bool,
    pub key_frame_request_delay: u32,
    pub app_data: AppData,
}

impl ProducerOptions {
    pub fn new(kind: MediaKind, rtp_parameters: RtpParameters) -> Self {
        Self {
            id: None,
            kind,
            rtp_parameters,
            paused: false,
            key_frame_request_delay: 0,
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug)]
pub struct Producer {}
