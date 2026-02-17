//! Collection of RTP-related data structures that are used to specify codec parameters and
//! capabilities of various endpoints.

use crate::fbs::{FromFbs, ToFbs, TryFromFbs};
use mediasoup_sys::fbs::rtp_parameters;
use mediasoup_types::rtp_parameters::*;
use std::borrow::Cow;
use std::error::Error;
use std::str::FromStr;

impl<'a> TryFromFbs<'a> for RtpParameters {
    type FbsType = rtp_parameters::RtpParametersRef<'a>;
    type Error = Box<dyn Error + Send + Sync>;

    fn try_from_fbs(rtp_parameters: Self::FbsType) -> Result<Self, Self::Error> {
        Ok(Self {
            mid: rtp_parameters.mid()?.map(|mid| mid.to_string()),
            codecs: rtp_parameters
                .codecs()?
                .into_iter()
                .map(|codec| {
                    let parameters = codec?
                        .parameters()?
                        .unwrap_or(planus::Vector::new_empty())
                        .into_iter()
                        .map(|parameters| {
                            Ok((
                                Cow::Owned(parameters?.name()?.to_string()),
                                match parameters?.value()? {
                                    rtp_parameters::ValueRef::Boolean(_)
                                    | rtp_parameters::ValueRef::Double(_)
                                    | rtp_parameters::ValueRef::Integer32Array(_) => {
                                        // TODO: Above value variant should not exist in the
                                        //  first place
                                        panic!("Invalid parameter")
                                    }
                                    rtp_parameters::ValueRef::Integer32(n) => {
                                        RtpCodecParametersParametersValue::Number(
                                            n.value()?.try_into()?,
                                        )
                                    }
                                    rtp_parameters::ValueRef::String(s) => {
                                        RtpCodecParametersParametersValue::String(
                                            s.value()?.to_string().into(),
                                        )
                                    }
                                },
                            ))
                        })
                        .collect::<Result<_, Box<dyn Error + Send + Sync>>>()?;
                    let rtcp_feedback = codec?
                        .rtcp_feedback()?
                        .unwrap_or(planus::Vector::new_empty())
                        .into_iter()
                        .map(|rtcp_feedback| {
                            Ok(RtcpFeedback::from_type_parameter(
                                rtcp_feedback?.type_()?,
                                rtcp_feedback?.parameter()?.unwrap_or_default(),
                            )?)
                        })
                        .collect::<Result<_, Box<dyn Error + Send + Sync>>>()?;

                    Ok(match MimeType::from_str(codec?.mime_type()?)? {
                        MimeType::Audio(mime_type) => RtpCodecParameters::Audio {
                            mime_type,
                            payload_type: codec?.payload_type()?,
                            clock_rate: codec?.clock_rate()?.try_into()?,
                            channels: codec?
                                .channels()?
                                .ok_or("Audio must have channels specified")?
                                .try_into()?,
                            parameters,
                            rtcp_feedback: vec![],
                        },
                        MimeType::Video(mime_type) => RtpCodecParameters::Video {
                            mime_type,
                            payload_type: codec?.payload_type()?,
                            clock_rate: codec?.clock_rate()?.try_into()?,
                            parameters,
                            rtcp_feedback,
                        },
                    })
                })
                .collect::<Result<_, Box<dyn Error + Send + Sync>>>()?,
            header_extensions: rtp_parameters
                .header_extensions()?
                .into_iter()
                .map(|header_extension_parameters| {
                    Ok(RtpHeaderExtensionParameters {
                        uri: RtpHeaderExtensionUri::from_fbs(&header_extension_parameters?.uri()?),
                        id: u16::from(header_extension_parameters?.id()?),
                        encrypt: header_extension_parameters?.encrypt()?,
                    })
                })
                .collect::<Result<_, Box<dyn Error + Send + Sync>>>()?,
            encodings: rtp_parameters
                .encodings()?
                .into_iter()
                .map(|encoding| {
                    Ok(RtpEncodingParameters {
                        ssrc: encoding?.ssrc()?,
                        rid: encoding?.rid()?.map(|rid| rid.to_string()),
                        codec_payload_type: encoding?.codec_payload_type()?,
                        rtx: encoding?.rtx()?.map(|rtx| RtpEncodingParametersRtx {
                            ssrc: rtx.ssrc().unwrap(),
                        }),
                        dtx: {
                            match encoding?.dtx()? {
                                true => Some(true),
                                false => None,
                            }
                        },
                        scalability_mode: encoding?
                            .scalability_mode()?
                            .unwrap_or(String::from("S1T1").as_str())
                            .parse()?,
                        max_bitrate: encoding?.max_bitrate()?,
                    })
                })
                .collect::<Result<_, Box<dyn Error + Send + Sync>>>()?,
            rtcp: RtcpParameters {
                cname: rtp_parameters
                    .rtcp()?
                    .cname()?
                    .map(|cname| cname.to_string()),
                reduced_size: rtp_parameters.rtcp()?.reduced_size()?,
            },
            msid: rtp_parameters.msid()?.map(|msid| msid.to_string()),
        })
    }
}

impl<'a> TryFromFbs<'a> for RtpEncodingParameters {
    type FbsType = rtp_parameters::RtpEncodingParametersRef<'a>;
    type Error = Box<dyn Error + Send + Sync>;

    fn try_from_fbs(encoding_parameters: Self::FbsType) -> Result<Self, Self::Error> {
        Ok(Self {
            ssrc: encoding_parameters.ssrc()?,
            rid: encoding_parameters.rid()?.map(|rid| rid.to_string()),
            codec_payload_type: encoding_parameters.codec_payload_type()?,
            rtx: if let Some(rtx) = encoding_parameters.rtx()? {
                Some(RtpEncodingParametersRtx { ssrc: rtx.ssrc()? })
            } else {
                None
            },
            dtx: {
                match encoding_parameters.dtx()? {
                    true => Some(true),
                    false => None,
                }
            },
            scalability_mode: encoding_parameters
                .scalability_mode()?
                .map(|maybe_scalability_mode| maybe_scalability_mode.parse())
                .transpose()?
                .unwrap_or_default(),
            max_bitrate: encoding_parameters.max_bitrate()?,
        })
    }
}

impl FromFbs for MediaKind {
    type FbsType = rtp_parameters::MediaKind;

    fn from_fbs(kind: &Self::FbsType) -> Self {
        match kind {
            rtp_parameters::MediaKind::Audio => MediaKind::Audio,
            rtp_parameters::MediaKind::Video => MediaKind::Video,
        }
    }
}

impl ToFbs for MediaKind {
    type FbsType = rtp_parameters::MediaKind;

    fn to_fbs(&self) -> Self::FbsType {
        match self {
            MediaKind::Audio => rtp_parameters::MediaKind::Audio,
            MediaKind::Video => rtp_parameters::MediaKind::Video,
        }
    }
}

impl FromFbs for RtpHeaderExtensionUri {
    type FbsType = rtp_parameters::RtpHeaderExtensionUri;

    fn from_fbs(uri: &Self::FbsType) -> Self {
        match uri {
            rtp_parameters::RtpHeaderExtensionUri::Mid => RtpHeaderExtensionUri::Mid,
            rtp_parameters::RtpHeaderExtensionUri::RtpStreamId => {
                RtpHeaderExtensionUri::RtpStreamId
            }
            rtp_parameters::RtpHeaderExtensionUri::RepairRtpStreamId => {
                RtpHeaderExtensionUri::RepairRtpStreamId
            }
            rtp_parameters::RtpHeaderExtensionUri::AbsSendTime => {
                RtpHeaderExtensionUri::AbsSendTime
            }
            rtp_parameters::RtpHeaderExtensionUri::TransportWideCcDraft01 => {
                RtpHeaderExtensionUri::TransportWideCcDraft01
            }
            rtp_parameters::RtpHeaderExtensionUri::SsrcAudioLevel => {
                RtpHeaderExtensionUri::SsrcAudioLevel
            }
            rtp_parameters::RtpHeaderExtensionUri::DependencyDescriptor => {
                RtpHeaderExtensionUri::DependencyDescriptor
            }
            rtp_parameters::RtpHeaderExtensionUri::VideoOrientation => {
                RtpHeaderExtensionUri::VideoOrientation
            }
            rtp_parameters::RtpHeaderExtensionUri::TimeOffset => RtpHeaderExtensionUri::TimeOffset,
            rtp_parameters::RtpHeaderExtensionUri::AbsCaptureTime => {
                RtpHeaderExtensionUri::AbsCaptureTime
            }
            rtp_parameters::RtpHeaderExtensionUri::PlayoutDelay => {
                RtpHeaderExtensionUri::PlayoutDelay
            }
            rtp_parameters::RtpHeaderExtensionUri::MediasoupPacketId => {
                RtpHeaderExtensionUri::MediasoupPacketId
            }
        }
    }
}

impl ToFbs for RtpHeaderExtensionUri {
    type FbsType = rtp_parameters::RtpHeaderExtensionUri;

    fn to_fbs(&self) -> Self::FbsType {
        match self {
            RtpHeaderExtensionUri::Mid => rtp_parameters::RtpHeaderExtensionUri::Mid,
            RtpHeaderExtensionUri::RtpStreamId => {
                rtp_parameters::RtpHeaderExtensionUri::RtpStreamId
            }
            RtpHeaderExtensionUri::RepairRtpStreamId => {
                rtp_parameters::RtpHeaderExtensionUri::RepairRtpStreamId
            }
            RtpHeaderExtensionUri::AbsSendTime => {
                rtp_parameters::RtpHeaderExtensionUri::AbsSendTime
            }
            RtpHeaderExtensionUri::TransportWideCcDraft01 => {
                rtp_parameters::RtpHeaderExtensionUri::TransportWideCcDraft01
            }
            RtpHeaderExtensionUri::SsrcAudioLevel => {
                rtp_parameters::RtpHeaderExtensionUri::SsrcAudioLevel
            }
            RtpHeaderExtensionUri::DependencyDescriptor => {
                rtp_parameters::RtpHeaderExtensionUri::DependencyDescriptor
            }
            RtpHeaderExtensionUri::VideoOrientation => {
                rtp_parameters::RtpHeaderExtensionUri::VideoOrientation
            }
            RtpHeaderExtensionUri::TimeOffset => rtp_parameters::RtpHeaderExtensionUri::TimeOffset,
            RtpHeaderExtensionUri::AbsCaptureTime => {
                rtp_parameters::RtpHeaderExtensionUri::AbsCaptureTime
            }
            RtpHeaderExtensionUri::PlayoutDelay => {
                rtp_parameters::RtpHeaderExtensionUri::PlayoutDelay
            }
            RtpHeaderExtensionUri::MediasoupPacketId => {
                rtp_parameters::RtpHeaderExtensionUri::MediasoupPacketId
            }
            RtpHeaderExtensionUri::Unsupported => panic!("Invalid RTP extension header URI"),
        }
    }
}

impl ToFbs for RtpParameters {
    type FbsType = rtp_parameters::RtpParameters;

    fn to_fbs(&self) -> rtp_parameters::RtpParameters {
        rtp_parameters::RtpParameters {
            mid: self.mid.clone(),
            codecs: self
                .codecs
                .iter()
                .map(|codec| rtp_parameters::RtpCodecParameters {
                    mime_type: codec.mime_type().as_str().to_string(),
                    payload_type: codec.payload_type(),
                    clock_rate: codec.clock_rate().get(),
                    channels: match &codec {
                        RtpCodecParameters::Audio { channels, .. } => Some(channels.get()),
                        RtpCodecParameters::Video { .. } => None,
                    },
                    parameters: Some(
                        codec
                            .parameters()
                            .iter()
                            .map(|(name, value)| rtp_parameters::Parameter {
                                name: name.to_string(),
                                value: match value {
                                    RtpCodecParametersParametersValue::String(s) => {
                                        rtp_parameters::Value::String(Box::new(
                                            rtp_parameters::String {
                                                value: s.to_string(),
                                            },
                                        ))
                                    }
                                    RtpCodecParametersParametersValue::Number(n) => {
                                        rtp_parameters::Value::Integer32(Box::new(
                                            rtp_parameters::Integer32 { value: *n as i32 },
                                        ))
                                    }
                                },
                            })
                            .collect(),
                    ),
                    rtcp_feedback: Some(
                        codec
                            .rtcp_feedback()
                            .iter()
                            .map(|rtcp_feedback| {
                                let (r#type, parameter) = rtcp_feedback.as_type_parameter();
                                rtp_parameters::RtcpFeedback {
                                    type_: r#type.to_string(),
                                    parameter: Some(parameter.to_string()),
                                }
                            })
                            .collect(),
                    ),
                })
                .collect(),
            header_extensions: self
                .header_extensions
                .iter()
                .map(
                    |header_extension_parameters| rtp_parameters::RtpHeaderExtensionParameters {
                        uri: header_extension_parameters.uri.to_fbs(),
                        id: header_extension_parameters.id as u8,
                        encrypt: header_extension_parameters.encrypt,
                        parameters: None,
                    },
                )
                .collect(),
            encodings: self
                .encodings
                .iter()
                .map(|encoding| rtp_parameters::RtpEncodingParameters {
                    ssrc: encoding.ssrc,
                    rid: encoding.rid.clone(),
                    codec_payload_type: encoding.codec_payload_type,
                    rtx: encoding
                        .rtx
                        .map(|rtx| Box::new(rtp_parameters::Rtx { ssrc: rtx.ssrc })),
                    dtx: encoding.dtx.unwrap_or_default(),
                    scalability_mode: if encoding.scalability_mode.is_none() {
                        None
                    } else {
                        Some(encoding.scalability_mode.as_str().to_string())
                    },
                    max_bitrate: encoding.max_bitrate,
                })
                .collect(),
            rtcp: Box::new(rtp_parameters::RtcpParameters {
                cname: self.rtcp.cname.clone(),
                reduced_size: self.rtcp.reduced_size,
            }),
            msid: self.msid.clone(),
        }
    }
}

impl ToFbs for RtpEncodingParameters {
    type FbsType = rtp_parameters::RtpEncodingParameters;

    fn to_fbs(&self) -> Self::FbsType {
        rtp_parameters::RtpEncodingParameters {
            ssrc: self.ssrc,
            rid: self.rid.clone(),
            codec_payload_type: self.codec_payload_type,
            rtx: self
                .rtx
                .map(|rtx| Box::new(rtp_parameters::Rtx { ssrc: rtx.ssrc })),
            dtx: self.dtx.unwrap_or_default(),
            scalability_mode: if self.scalability_mode.is_none() {
                None
            } else {
                Some(self.scalability_mode.as_str().to_string())
            },
            max_bitrate: self.max_bitrate,
        }
    }
}
