use crate::rtp_parameters::{
    MimeType, MimeTypeAudio, MimeTypeVideo, RtpCapabilities, RtpCodecCapability,
    RtpCodecParameters, RtpCodecParametersParametersValue, RtpParameters,
};
use rand::rngs::SmallRng;
use rand::{Rng, SeedableRng};
use std::collections::BTreeMap;
use thiserror::Error;

#[derive(Debug, Default)]
pub(crate) struct RtpMappingCodec {
    payload_type: u8,
    mapped_payload_type: u8,
}

#[derive(Debug, Default)]
pub(crate) struct RtpMappingEncoding {
    ssrc: Option<u32>,
    rid: Option<String>,
    // TODO: Maybe enum?
    scalability_mode: Option<String>,
    mapped_ssrc: u32,
}

#[derive(Debug, Default)]
pub(crate) struct RtpMapping {
    codecs: Vec<RtpMappingCodec>,
    encodings: Vec<RtpMappingEncoding>,
}

#[derive(Debug, Error)]
pub enum RtpParametersError {
    #[error("invalid codec apt parameter {0}")]
    InvalidAptParameter(String),
}

/// Validates RtpParameters.
pub(crate) fn validate_rtp_parameters(
    rtp_parameters: &RtpParameters,
) -> Result<(), RtpParametersError> {
    for codec in rtp_parameters.codecs.iter() {
        validate_rtp_codec_parameters(codec)?;
    }

    Ok(())
}

/// Validates RtpCodecParameters. It may modify given data by adding missing
/// fields with default values.
/// It throws if invalid.
fn validate_rtp_codec_parameters(codec: &RtpCodecParameters) -> Result<(), RtpParametersError> {
    let parameters = match codec {
        RtpCodecParameters::Audio { parameters, .. } => parameters,
        RtpCodecParameters::Video { parameters, .. } => parameters,
    };

    for (key, value) in parameters {
        // Specific parameters validation.
        if key.as_str() == "apt" {
            match value {
                RtpCodecParametersParametersValue::Number(_) => {
                    // Good
                }
                RtpCodecParametersParametersValue::String(string) => {
                    return Err(RtpParametersError::InvalidAptParameter(string.clone()));
                }
            }
        }
    }

    Ok(())
}

#[derive(Debug, Error)]
pub enum RtpParametersMappingError {
    #[error("unsupported codec [mimeType:{mime_type:?}, payloadType:{payload_type}]")]
    UnsupportedCodec {
        mime_type: MimeType,
        payload_type: u8,
    },
    #[error("no RTX codec for capability codec PT {preferred_payload_type:?}")]
    UnsupportedRTXCodec { preferred_payload_type: Option<u8> },
    #[error("missing media codec found for RTX PT {payload_type}")]
    MissingMediaCodecForRTX { payload_type: u8 },
}

/**
 * Get a mapping of codec payloads and encodings of the given Producer RTP
 * parameters as values expected by the Router.
 *
 * Returns `Err()` if invalid or non supported RTP parameters are given.
 */
pub(crate) fn get_producer_rtp_parameters_mapping(
    rtp_parameters: &RtpParameters,
    rtp_capabilities: &RtpCapabilities,
) -> Result<RtpMapping, RtpParametersMappingError> {
    let mut rtp_mapping = RtpMapping::default();

    // Match parameters media codecs to capabilities media codecs.
    let mut codec_to_cap_codec = BTreeMap::<&RtpCodecParameters, &RtpCodecCapability>::new();

    for codec in rtp_parameters.codecs.iter() {
        if codec.is_rtx() {
            continue;
        }

        // Search for the same media codec in capabilities.
        match rtp_capabilities
            .codecs
            .iter()
            .find(|cap_codec| match_codecs(codec.into(), (*cap_codec).into(), true, true))
        {
            Some(matched_codec_capability) => {
                codec_to_cap_codec.insert(codec, matched_codec_capability);
            }
            None => {
                return Err(RtpParametersMappingError::UnsupportedCodec {
                    mime_type: codec.mime_type(),
                    payload_type: codec.payload_type(),
                });
            }
        }
    }

    // Match parameters RTX codecs to capabilities RTX codecs.
    for codec in rtp_parameters.codecs.iter() {
        if !codec.is_rtx() {
            continue;
        }

        // Search for the associated media codec.
        let associated_media_codec = rtp_parameters.codecs.iter().find(|media_codec| {
            let media_codec_payload_type = media_codec.payload_type();
            let codec_parameters_apt = codec.parameters().get(&"apt".to_string());

            match codec_parameters_apt {
                Some(RtpCodecParametersParametersValue::Number(apt)) => {
                    media_codec_payload_type as u32 == *apt
                }
                _ => false,
            }
        });

        match associated_media_codec {
            Some(associated_media_codec) => {
                let cap_media_codec = codec_to_cap_codec.get(associated_media_codec).unwrap();

                // Ensure that the capabilities media codec has a RTX codec.
                let associated_cap_rtx_codec = rtp_capabilities.codecs.iter().find(|cap_codec| {
                    if !cap_codec.is_rtx() {
                        return false;
                    }

                    let preferred_payload_type = match cap_media_codec.preferred_payload_type() {
                        Some(preferred_payload_type) => preferred_payload_type,
                        None => {
                            return false;
                        }
                    };

                    let cap_codec_parameters_apt = cap_codec.parameters().get(&"apt".to_string());
                    match cap_codec_parameters_apt {
                        Some(RtpCodecParametersParametersValue::Number(apt)) => {
                            preferred_payload_type as u32 == *apt
                        }
                        _ => false,
                    }
                });

                match associated_cap_rtx_codec {
                    Some(associated_cap_rtx_codec) => {
                        codec_to_cap_codec.insert(codec, associated_cap_rtx_codec);
                    }
                    None => {
                        return Err(RtpParametersMappingError::UnsupportedRTXCodec {
                            preferred_payload_type: cap_media_codec.preferred_payload_type(),
                        });
                    }
                }
            }
            None => {
                return Err(RtpParametersMappingError::MissingMediaCodecForRTX {
                    payload_type: codec.payload_type(),
                });
            }
        }
    }

    // Generate codecs mapping.
    for (codec, cap_codec) in codec_to_cap_codec {
        rtp_mapping.codecs.push(RtpMappingCodec {
            payload_type: codec.payload_type(),
            mapped_payload_type: cap_codec.preferred_payload_type().unwrap(),
        });
    }

    // Generate encodings mapping.
    let mut mapped_ssrc: u32 = SmallRng::from_entropy().gen_range(100000000, 999999999);

    for encoding in rtp_parameters.encodings.iter() {
        rtp_mapping.encodings.push(RtpMappingEncoding {
            ssrc: encoding.ssrc,
            rid: encoding.rid.clone(),
            scalability_mode: encoding.scalability_mode.clone(),
            mapped_ssrc,
        });

        mapped_ssrc += 1;
    }

    Ok(rtp_mapping)
}

struct CodecToMatch<'a> {
    channels: Option<u8>,
    clock_rate: u32,
    mime_type: MimeType,
    parameters: &'a BTreeMap<String, RtpCodecParametersParametersValue>,
}

impl<'a> From<&'a RtpCodecCapability> for CodecToMatch<'a> {
    fn from(rtp_codec_capability: &'a RtpCodecCapability) -> Self {
        match rtp_codec_capability {
            RtpCodecCapability::Audio {
                mime_type,
                channels,
                clock_rate,
                parameters,
                ..
            } => Self {
                channels: Some(*channels),
                clock_rate: *clock_rate,
                mime_type: MimeType::Audio(*mime_type),
                parameters,
            },
            RtpCodecCapability::Video {
                mime_type,
                clock_rate,
                parameters,
                ..
            } => Self {
                channels: None,
                clock_rate: *clock_rate,
                mime_type: MimeType::Video(*mime_type),
                parameters,
            },
        }
    }
}
impl<'a> From<&'a RtpCodecParameters> for CodecToMatch<'a> {
    fn from(rtp_codec_parameters: &'a RtpCodecParameters) -> Self {
        match rtp_codec_parameters {
            RtpCodecParameters::Audio {
                mime_type,
                channels,
                clock_rate,
                parameters,
                ..
            } => Self {
                channels: Some(*channels),
                clock_rate: *clock_rate,
                mime_type: MimeType::Audio(*mime_type),
                parameters,
            },
            RtpCodecParameters::Video {
                mime_type,
                clock_rate,
                parameters,
                ..
            } => Self {
                channels: None,
                clock_rate: *clock_rate,
                mime_type: MimeType::Video(*mime_type),
                parameters,
            },
        }
    }
}

fn match_codecs(codec_a: CodecToMatch, codec_b: CodecToMatch, strict: bool, modify: bool) -> bool {
    if codec_a.mime_type != codec_b.mime_type {
        return false;
    }

    if codec_a.channels != codec_b.channels {
        return false;
    }

    if codec_a.clock_rate != codec_b.clock_rate {
        return false;
    }
    // Per codec special checks.
    match codec_a.mime_type {
        MimeType::Video(MimeTypeVideo::H264) => {
            let packetization_mode_a = codec_a
                .parameters
                .get("packetization-mode")
                .unwrap_or(&RtpCodecParametersParametersValue::Number(0));
            let packetization_mode_b = codec_b
                .parameters
                .get("packetization-mode")
                .unwrap_or(&RtpCodecParametersParametersValue::Number(0));

            if packetization_mode_a != packetization_mode_b {
                return false;
            }

            // If strict matching check profile-level-id.
            if strict {
                // TODO: port h264-profile-level-id to Rust and implement these checks
                // if (!h264.isSameProfile(codec_a.parameters, codec_b.parameters))
                //   return false;
                //
                // let selected_profile_level_id;
                //
                // try
                // {
                //   selected_profile_level_id =
                //     h264.generateProfileLevelIdForAnswer(codec_a.parameters, codec_b.parameters);
                // }
                // catch (error)
                // {
                //   return false;
                // }
                //
                // if modify {
                //   if selected_profile_level_id {
                //     codec_a.parameters.get_mut("profile-level-id") = selected_profile_level_id;
                //     } else {
                //     delete codec_a.parameters['profile-level-id'];
                //     }
                // }
            }
        }

        MimeType::Video(MimeTypeVideo::VP9) => {
            // If strict matching check profile-id.
            if strict {
                let profile_id_a = codec_a
                    .parameters
                    .get("profile-id")
                    .unwrap_or(&RtpCodecParametersParametersValue::Number(0));
                let profile_id_b = codec_b
                    .parameters
                    .get("profile-id")
                    .unwrap_or(&RtpCodecParametersParametersValue::Number(0));

                if profile_id_a != profile_id_b {
                    return false;
                }
            }
        }

        _ => {}
    }

    return true;
}
