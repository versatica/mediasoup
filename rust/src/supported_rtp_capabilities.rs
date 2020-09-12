use crate::rtp_parameters::{
    MediaKind, MimeTypeAudio, MimeTypeVideo, RtcpFeedback, RtpCapabilities, RtpCodecCapability,
    RtpCodecParametersParametersValue, RtpHeaderExtension, RtpHeaderExtensionDirection,
};
use std::collections::BTreeMap;

pub fn get_supported_rtp_capabilities() -> RtpCapabilities {
    RtpCapabilities {
        codecs: vec![
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::Opus,
                preferred_payload_type: None,
                clock_rate: 48000,
                channels: 2,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::PCMU,
                preferred_payload_type: Some(0),
                clock_rate: 8000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::PCMA,
                preferred_payload_type: Some(8),
                clock_rate: 8000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::ISAC,
                preferred_payload_type: None,
                clock_rate: 32000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::ISAC,
                preferred_payload_type: None,
                clock_rate: 16000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::G722,
                preferred_payload_type: Some(9),
                clock_rate: 8000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::iLBC,
                preferred_payload_type: None,
                clock_rate: 8000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::SILK,
                preferred_payload_type: None,
                clock_rate: 24000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::SILK,
                preferred_payload_type: None,
                clock_rate: 16000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::SILK,
                preferred_payload_type: None,
                clock_rate: 12000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::SILK,
                preferred_payload_type: None,
                clock_rate: 8000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![RtcpFeedback {
                    r#type: "transport-cc".to_string(),
                    parameter: "".to_string(),
                }],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::CN,
                preferred_payload_type: Some(13),
                clock_rate: 32000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::CN,
                preferred_payload_type: Some(13),
                clock_rate: 16000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::CN,
                preferred_payload_type: Some(13),
                clock_rate: 8000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::TelephoneEvent,
                preferred_payload_type: None,
                clock_rate: 48000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::TelephoneEvent,
                preferred_payload_type: None,
                clock_rate: 32000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::TelephoneEvent,
                preferred_payload_type: None,
                clock_rate: 16000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Audio {
                mime_type: MimeTypeAudio::TelephoneEvent,
                preferred_payload_type: None,
                clock_rate: 8000,
                channels: 1,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![],
            },
            RtpCodecCapability::Video {
                mime_type: MimeTypeVideo::VP8,
                preferred_payload_type: None,
                clock_rate: 90000,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "pli".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "ccm".to_string(),
                        parameter: "fir".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "goog-remb".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "transport-cc".to_string(),
                        parameter: "".to_string(),
                    },
                ],
            },
            RtpCodecCapability::Video {
                mime_type: MimeTypeVideo::VP9,
                preferred_payload_type: None,
                clock_rate: 90000,
                parameters: BTreeMap::new(),
                rtcp_feedback: vec![
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "pli".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "ccm".to_string(),
                        parameter: "fir".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "goog-remb".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "transport-cc".to_string(),
                        parameter: "".to_string(),
                    },
                ],
            },
            RtpCodecCapability::Video {
                mime_type: MimeTypeVideo::H264,
                preferred_payload_type: None,
                clock_rate: 90000,
                parameters: {
                    let mut parameters = BTreeMap::new();
                    parameters.insert(
                        "packetization-mode".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters.insert(
                        "level-asymmetry-allowed".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters
                },
                rtcp_feedback: vec![
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "pli".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "ccm".to_string(),
                        parameter: "fir".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "goog-remb".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "transport-cc".to_string(),
                        parameter: "".to_string(),
                    },
                ],
            },
            RtpCodecCapability::Video {
                mime_type: MimeTypeVideo::H264,
                preferred_payload_type: None,
                clock_rate: 90000,
                parameters: {
                    let mut parameters = BTreeMap::new();
                    parameters.insert(
                        "packetization-mode".to_string(),
                        RtpCodecParametersParametersValue::Number(0),
                    );
                    parameters.insert(
                        "level-asymmetry-allowed".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters
                },
                rtcp_feedback: vec![
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "pli".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "ccm".to_string(),
                        parameter: "fir".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "goog-remb".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "transport-cc".to_string(),
                        parameter: "".to_string(),
                    },
                ],
            },
            RtpCodecCapability::Video {
                mime_type: MimeTypeVideo::H265,
                preferred_payload_type: None,
                clock_rate: 90000,
                parameters: {
                    let mut parameters = BTreeMap::new();
                    parameters.insert(
                        "packetization-mode".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters.insert(
                        "level-asymmetry-allowed".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters
                },
                rtcp_feedback: vec![
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "pli".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "ccm".to_string(),
                        parameter: "fir".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "goog-remb".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "transport-cc".to_string(),
                        parameter: "".to_string(),
                    },
                ],
            },
            RtpCodecCapability::Video {
                mime_type: MimeTypeVideo::H265,
                preferred_payload_type: None,
                clock_rate: 90000,
                parameters: {
                    let mut parameters = BTreeMap::new();
                    parameters.insert(
                        "packetization-mode".to_string(),
                        RtpCodecParametersParametersValue::Number(0),
                    );
                    parameters.insert(
                        "level-asymmetry-allowed".to_string(),
                        RtpCodecParametersParametersValue::Number(1),
                    );
                    parameters
                },
                rtcp_feedback: vec![
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "nack".to_string(),
                        parameter: "pli".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "ccm".to_string(),
                        parameter: "fir".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "goog-remb".to_string(),
                        parameter: "".to_string(),
                    },
                    RtcpFeedback {
                        r#type: "transport-cc".to_string(),
                        parameter: "".to_string(),
                    },
                ],
            },
        ],
        header_extensions: vec![
            RtpHeaderExtension {
                kind: Some(MediaKind::Audio),
                uri: "urn:ietf:params:rtp-hdrext:sdes:mid".to_string(),
                preferred_id: 1,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "urn:ietf:params:rtp-hdrext:sdes:mid".to_string(),
                preferred_id: 1,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id".to_string(),
                preferred_id: 2,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::RecvOnly,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id".to_string(),
                preferred_id: 3,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::RecvOnly,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Audio),
                uri: "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time".to_string(),
                preferred_id: 4,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time".to_string(),
                preferred_id: 4,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            // NOTE: For audio we just enable transport-wide-cc-01 when receiving media.
            RtpHeaderExtension {
                kind: Some(MediaKind::Audio),
                uri: "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"
                    .to_string(),
                preferred_id: 5,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::RecvOnly,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"
                    .to_string(),
                preferred_id: 5,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            // NOTE: Remove this once framemarking draft becomes RFC.
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07".to_string(),
                preferred_id: 6,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "urn:ietf:params:rtp-hdrext:framemarking".to_string(),
                preferred_id: 7,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Audio),
                uri: "urn:ietf:params:rtp-hdrext:ssrc-audio-level".to_string(),
                preferred_id: 10,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "urn:3gpp:video-orientation".to_string(),
                preferred_id: 11,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
            RtpHeaderExtension {
                kind: Some(MediaKind::Video),
                uri: "urn:ietf:params:rtp-hdrext:toffset".to_string(),
                preferred_id: 12,
                preferred_encrypt: false,
                direction: RtpHeaderExtensionDirection::SendRecv,
            },
        ],
        fec_mechanisms: vec![],
    }
}
