use crate::rtp_parameters::{
    MediaKind, MimeType, MimeTypeVideo, RtcpParameters, RtpCapabilities, RtpCapabilitiesFinalized,
    RtpCodecCapability, RtpCodecCapabilityFinalized, RtpCodecParameters,
    RtpCodecParametersParametersValue, RtpEncodingParameters, RtpEncodingParametersRtx,
    RtpHeaderExtensionDirection, RtpHeaderExtensionParameters, RtpParameters,
};
use crate::{scalability_modes, supported_rtp_capabilities};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::mem;
use std::ops::Deref;
use thiserror::Error;

const DYNAMIC_PAYLOAD_TYPES: &[u8] = &[
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118,
    119, 120, 121, 122, 123, 124, 125, 126, 127, 96, 97, 98, 99,
];

const TWCC_HEADER: &str =
    "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01";
const ABS_SEND_TIME_HEADER: &str = "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time";

#[derive(Debug, Default, Clone, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RtpMappingCodec {
    pub payload_type: u8,
    pub mapped_payload_type: u8,
}

#[derive(Debug, Default, Clone, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RtpMappingEncoding {
    pub ssrc: Option<u32>,
    pub rid: Option<String>,
    // TODO: Maybe enum?
    pub scalability_mode: Option<String>,
    pub mapped_ssrc: u32,
}

#[derive(Debug, Default, Clone, Deserialize, Serialize)]
pub struct RtpMapping {
    pub codecs: Vec<RtpMappingCodec>,
    pub encodings: Vec<RtpMappingEncoding>,
}

#[derive(Debug, Error)]
pub enum RtpParametersError {
    #[error("invalid codec apt parameter {0}")]
    InvalidAptParameter(String),
}

#[derive(Debug, Error)]
pub enum RtpCapabilitiesError {
    #[error("media codec not supported [mime_type:{mime_type:?}")]
    UnsupportedCodec { mime_type: MimeType },
    #[error("cannot allocate more dynamic codec payload types")]
    CannotAllocate,
    #[error("invalid codec apt parameter {0}")]
    InvalidAptParameter(String),
    #[error("duplicated {0}")]
    DuplicatedPreferredPayloadType(u8),
}

#[derive(Debug, Error)]
pub enum RtpParametersMappingError {
    #[error("unsupported codec [mime_type:{mime_type:?}, payloadType:{payload_type}]")]
    UnsupportedCodec {
        mime_type: MimeType,
        payload_type: u8,
    },
    #[error("no RTX codec for capability codec PT {preferred_payload_type}")]
    UnsupportedRTXCodec { preferred_payload_type: u8 },
    #[error("missing media codec found for RTX PT {payload_type}")]
    MissingMediaCodecForRTX { payload_type: u8 },
}

#[derive(Debug, Error)]
pub enum ConsumerRtpParametersError {
    #[error("invalid capabilities: {0}")]
    InvalidCapabilities(RtpCapabilitiesError),
    #[error("no compatible media codecs")]
    NoCompatibleMediaCodecs,
}

fn generate_ssrc() -> u32 {
    fastrand::u32(100000000..999999999)
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

/// Validates RtpCodecParameters.
fn validate_rtp_codec_parameters(codec: &RtpCodecParameters) -> Result<(), RtpParametersError> {
    for (key, value) in codec.parameters() {
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

// Validates RtpCodecCapability.
fn validate_rtp_codec_capability(codec: &RtpCodecCapability) -> Result<(), RtpCapabilitiesError> {
    for (key, value) in codec.parameters() {
        // Specific parameters validation.
        if key.as_str() == "apt" {
            match value {
                RtpCodecParametersParametersValue::Number(_) => {
                    // Good
                }
                RtpCodecParametersParametersValue::String(string) => {
                    return Err(RtpCapabilitiesError::InvalidAptParameter(string.clone()));
                }
            }
        }
    }

    Ok(())
}

/// Validates RtpCapabilities.
pub(crate) fn validate_rtp_capabilities(
    caps: &RtpCapabilities,
) -> Result<(), RtpCapabilitiesError> {
    for codec in caps.codecs.iter() {
        validate_rtp_codec_capability(codec)?;
    }

    Ok(())
}

/// Generate RTP capabilities for the Router based on the given media codecs and mediasoup supported
/// RTP capabilities.
pub(crate) fn generate_router_rtp_capabilities(
    mut media_codecs: Vec<RtpCodecCapability>,
) -> Result<RtpCapabilitiesFinalized, RtpCapabilitiesError> {
    let supported_rtp_capabilities = supported_rtp_capabilities::get_supported_rtp_capabilities();

    validate_rtp_capabilities(&supported_rtp_capabilities)?;

    let mut dynamic_payload_types = Vec::from(DYNAMIC_PAYLOAD_TYPES);
    let mut caps = RtpCapabilitiesFinalized {
        codecs: vec![],
        header_extensions: supported_rtp_capabilities.header_extensions,
        fec_mechanisms: vec![],
    };

    for media_codec in media_codecs.iter_mut() {
        validate_rtp_codec_capability(media_codec)?;

        let codec = match supported_rtp_capabilities
            .codecs
            .iter()
            .find(|supported_codec| {
                match_codecs(
                    media_codec.deref().into(),
                    (*supported_codec).into(),
                    false,
                    false,
                )
            }) {
            Some(codec) => codec,
            None => {
                return Err(RtpCapabilitiesError::UnsupportedCodec {
                    mime_type: media_codec.mime_type(),
                });
            }
        };

        let preferred_payload_type = match media_codec.preferred_payload_type() {
            // If the given media codec has preferred_payload_type, keep it.
            Some(preferred_payload_type) => {
                // Also remove the payload_type from the list of available dynamic values.
                // TODO: drain_filter() would be nicer, but it is not stable yet
                let mut i = 0;
                while i != dynamic_payload_types.len() {
                    if dynamic_payload_types[i] == preferred_payload_type {
                        dynamic_payload_types.remove(i);
                        break;
                    } else {
                        i += 1;
                    }
                }

                preferred_payload_type
            }
            None => {
                match codec.preferred_payload_type() {
                    // Otherwise if the supported codec has preferredPayloadType, use it.
                    Some(preferred_payload_type) => {
                        // No need to remove it from the list since it's not a dynamic value.
                        preferred_payload_type
                    }
                    // Otherwise choose a dynamic one.
                    None => {
                        if dynamic_payload_types.is_empty() {
                            return Err(RtpCapabilitiesError::CannotAllocate);
                        }
                        // Take the first available payload type and remove it from the list.
                        dynamic_payload_types.remove(0)
                    }
                }
            }
        };

        // Ensure there is not duplicated preferredPayloadType values.
        for codec in caps.codecs.iter() {
            if codec.preferred_payload_type() == preferred_payload_type {
                return Err(RtpCapabilitiesError::DuplicatedPreferredPayloadType(
                    preferred_payload_type,
                ));
            }
        }

        let codec_finalized = match codec {
            RtpCodecCapability::Audio {
                mime_type,
                preferred_payload_type: _,
                clock_rate,
                channels,
                parameters,
                rtcp_feedback,
            } => RtpCodecCapabilityFinalized::Audio {
                mime_type: *mime_type,
                preferred_payload_type,
                clock_rate: *clock_rate,
                channels: *channels,
                parameters: {
                    // Merge the media codec parameters.
                    let mut parameters = parameters.clone();
                    parameters.extend(mem::take(media_codec.parameters_mut()));
                    parameters
                },
                rtcp_feedback: rtcp_feedback.clone(),
            },
            RtpCodecCapability::Video {
                mime_type,
                preferred_payload_type: _,
                clock_rate,
                parameters,
                rtcp_feedback,
            } => RtpCodecCapabilityFinalized::Video {
                mime_type: *mime_type,
                preferred_payload_type,
                clock_rate: *clock_rate,
                parameters: {
                    // Merge the media codec parameters.
                    let mut parameters = parameters.clone();
                    parameters.extend(mem::take(media_codec.parameters_mut()));
                    parameters
                },
                rtcp_feedback: rtcp_feedback.clone(),
            },
        };

        // Add a RTX video codec if video.
        if matches!(codec_finalized, RtpCodecCapabilityFinalized::Video {..}) {
            if dynamic_payload_types.is_empty() {
                return Err(RtpCapabilitiesError::CannotAllocate);
            }
            // Take the first available payload_type and remove it from the list.
            let payload_type = dynamic_payload_types.remove(0);

            let rtx_codec = RtpCodecCapabilityFinalized::Video {
                mime_type: MimeTypeVideo::RTX,
                preferred_payload_type: payload_type,
                clock_rate: codec_finalized.clock_rate(),
                parameters: {
                    let mut parameters = BTreeMap::new();
                    parameters.insert(
                        "apt".to_string(),
                        RtpCodecParametersParametersValue::Number(
                            codec_finalized.preferred_payload_type() as u32,
                        ),
                    );
                    parameters
                },
                rtcp_feedback: vec![],
            };

            // Append to the codec list.
            caps.codecs.push(codec_finalized);
            caps.codecs.push(rtx_codec);
        } else {
            // Append to the codec list.
            caps.codecs.push(codec_finalized);
        }
    }

    Ok(caps)
}

/**
 * Get a mapping of codec payloads and encodings of the given Producer RTP
 * parameters as values expected by the Router.
 *
 * Returns `Err()` if invalid or non supported RTP parameters are given.
 */
pub(crate) fn get_producer_rtp_parameters_mapping(
    rtp_parameters: &RtpParameters,
    rtp_capabilities: &RtpCapabilitiesFinalized,
) -> Result<RtpMapping, RtpParametersMappingError> {
    let mut rtp_mapping = RtpMapping::default();

    // Match parameters media codecs to capabilities media codecs.
    let mut codec_to_cap_codec =
        BTreeMap::<&RtpCodecParameters, &RtpCodecCapabilityFinalized>::new();

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

                    let cap_codec_parameters_apt = cap_codec.parameters().get(&"apt".to_string());
                    match cap_codec_parameters_apt {
                        Some(RtpCodecParametersParametersValue::Number(apt)) => {
                            cap_media_codec.preferred_payload_type() as u32 == *apt
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
            mapped_payload_type: cap_codec.preferred_payload_type(),
        });
    }

    // Generate encodings mapping.
    let mut mapped_ssrc: u32 = generate_ssrc();

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

// Generate RTP parameters to be internally used by Consumers given the RTP parameters of a Producer
// and the RTP capabilities of the Router.
pub(crate) fn get_consumable_rtp_parameters(
    kind: MediaKind,
    params: &RtpParameters,
    caps: &RtpCapabilitiesFinalized,
    rtp_mapping: &RtpMapping,
) -> RtpParameters {
    let mut consumable_params = RtpParameters::default();

    for codec in params.codecs.iter() {
        if codec.is_rtx() {
            continue;
        }

        let consumable_codec_pt = rtp_mapping
            .codecs
            .iter()
            .find(|entry| entry.payload_type == codec.payload_type())
            .unwrap()
            .mapped_payload_type;

        let consumable_codec = match caps
            .codecs
            .iter()
            .find(|cap_codec| cap_codec.preferred_payload_type() == consumable_codec_pt)
            .unwrap()
        {
            RtpCodecCapabilityFinalized::Audio {
                mime_type,
                preferred_payload_type,
                clock_rate,
                channels,
                parameters: _,
                rtcp_feedback,
            } => {
                RtpCodecParameters::Audio {
                    mime_type: *mime_type,
                    payload_type: *preferred_payload_type,
                    clock_rate: *clock_rate,
                    channels: *channels,
                    // Keep the Producer codec parameters.
                    parameters: codec.parameters().clone(),
                    rtcp_feedback: rtcp_feedback.clone(),
                }
            }
            RtpCodecCapabilityFinalized::Video {
                mime_type,
                preferred_payload_type,
                clock_rate,
                parameters: _,
                rtcp_feedback,
            } => {
                RtpCodecParameters::Video {
                    mime_type: *mime_type,
                    payload_type: *preferred_payload_type,
                    clock_rate: *clock_rate,
                    // Keep the Producer codec parameters.
                    parameters: codec.parameters().clone(),
                    rtcp_feedback: rtcp_feedback.clone(),
                }
            }
        };

        let consumable_cap_rtx_codec = caps.codecs.iter().find(|cap_rtx_codec| {
            if !cap_rtx_codec.is_rtx() {
                return false;
            }

            let cap_rtx_codec_parameters_apt = cap_rtx_codec.parameters().get(&"apt".to_string());

            match cap_rtx_codec_parameters_apt {
                Some(RtpCodecParametersParametersValue::Number(apt)) => {
                    *apt as u8 == consumable_codec.payload_type()
                }
                _ => false,
            }
        });

        consumable_params.codecs.push(consumable_codec);

        if let Some(consumable_cap_rtx_codec) = consumable_cap_rtx_codec {
            let consumable_rtx_codec = match consumable_cap_rtx_codec {
                RtpCodecCapabilityFinalized::Audio {
                    mime_type,
                    preferred_payload_type,
                    clock_rate,
                    channels,
                    parameters,
                    rtcp_feedback,
                } => RtpCodecParameters::Audio {
                    mime_type: *mime_type,
                    payload_type: *preferred_payload_type,
                    clock_rate: *clock_rate,
                    channels: *channels,
                    parameters: parameters.clone(),
                    rtcp_feedback: rtcp_feedback.clone(),
                },
                RtpCodecCapabilityFinalized::Video {
                    mime_type,
                    preferred_payload_type,
                    clock_rate,
                    parameters,
                    rtcp_feedback,
                } => RtpCodecParameters::Video {
                    mime_type: *mime_type,
                    payload_type: *preferred_payload_type,
                    clock_rate: *clock_rate,
                    parameters: parameters.clone(),
                    rtcp_feedback: rtcp_feedback.clone(),
                },
            };

            consumable_params.codecs.push(consumable_rtx_codec);
        }
    }

    for cap_ext in caps.header_extensions.iter() {
        // Just take RTP header extension that can be used in Consumers.
        match cap_ext.kind {
            Some(cap_ext_kind) => {
                if cap_ext_kind != kind {
                    continue;
                }
            }
            None => {
                // TODO: Should this really skip "any" extensions?
                continue;
            }
        }
        if !matches!(
            cap_ext.direction,
            RtpHeaderExtensionDirection::SendRecv | RtpHeaderExtensionDirection::SendOnly
        ) {
            continue;
        }

        let consumable_ext = RtpHeaderExtensionParameters {
            uri: cap_ext.uri.clone(),
            id: cap_ext.preferred_id,
            encrypt: cap_ext.preferred_encrypt,
            parameters: BTreeMap::new(),
        };

        consumable_params.header_extensions.push(consumable_ext);
    }

    for (consumable_encoding, mapped_ssrc) in params.encodings.iter().zip(
        rtp_mapping
            .encodings
            .iter()
            .map(|encoding| encoding.mapped_ssrc),
    ) {
        let mut consumable_encoding = consumable_encoding.clone();
        // Remove useless fields.
        consumable_encoding.rid.take();
        consumable_encoding.rtx.take();
        consumable_encoding.codec_payload_type.take();

        // Set the mapped ssrc.
        consumable_encoding.ssrc = Some(mapped_ssrc);

        consumable_params.encodings.push(consumable_encoding);
    }

    consumable_params.rtcp = RtcpParameters {
        cname: params.rtcp.cname.clone(),
        reduced_size: true,
        mux: Some(true),
    };

    return consumable_params;
}

/// Check whether the given RTP capabilities can consume the given Producer.
pub(crate) fn can_consume(
    consumable_params: &RtpParameters,
    caps: &RtpCapabilities,
) -> Result<bool, RtpCapabilitiesError> {
    validate_rtp_capabilities(&caps)?;

    let mut matching_codecs = Vec::<&RtpCodecParameters>::new();

    for codec in consumable_params.codecs.iter() {
        if caps
            .codecs
            .iter()
            .find(|cap_codec| match_codecs(cap_codec.deref().into(), codec.into(), true, false))
            .is_some()
        {
            matching_codecs.push(codec);
        }
    }

    // Ensure there is at least one media codec.
    Ok(matching_codecs
        .get(0)
        .map(|codec| !codec.is_rtx())
        .unwrap_or_default())
}

/// Generate RTP parameters for a specific Consumer.
///
/// It reduces encodings to just one and takes into account given RTP capabilities to reduce codecs,
/// codecs' RTCP feedback and header extensions, and also enables or disabled RTX.
pub(crate) fn get_consumer_rtp_parameters(
    consumable_params: &RtpParameters,
    caps: RtpCapabilities,
) -> Result<RtpParameters, ConsumerRtpParametersError> {
    let mut consumer_params = RtpParameters::default();
    consumer_params.rtcp = consumable_params.rtcp.clone();

    for cap_codec in caps.codecs.iter() {
        validate_rtp_codec_capability(cap_codec)
            .map_err(ConsumerRtpParametersError::InvalidCapabilities)?;
    }

    let mut rtx_supported = false;

    for mut codec in consumable_params.codecs.clone() {
        if let Some(matched_cap_codec) = caps
            .codecs
            .iter()
            .find(|cap_codec| match_codecs(cap_codec.deref().into(), (&codec).into(), true, false))
        {
            *codec.rtcp_feedback_mut() = matched_cap_codec.rtcp_feedback().clone();
            consumer_params.codecs.push(codec);
        }
    }

    // Must sanitize the list of matched codecs by removing useless RTX codecs.
    let mut remove_codecs = Vec::new();
    for (idx, codec) in consumer_params.codecs.iter().enumerate() {
        if codec.is_rtx() {
            // Search for the associated media codec.
            let associated_media_codec = consumer_params.codecs.iter().find(|media_codec| {
                match codec.parameters().get("apt") {
                    Some(RtpCodecParametersParametersValue::Number(apt)) => {
                        media_codec.payload_type() as u32 == *apt
                    }
                    _ => false,
                }
            });

            if associated_media_codec.is_some() {
                rtx_supported = true;
            } else {
                remove_codecs.push(idx);
            }
        }
    }
    for idx in remove_codecs.into_iter().rev() {
        consumer_params.codecs.remove(idx);
    }

    // Ensure there is at least one media codec.
    if consumer_params.codecs.is_empty() || consumer_params.codecs[0].is_rtx() {
        return Err(ConsumerRtpParametersError::NoCompatibleMediaCodecs);
    }

    consumer_params.header_extensions.retain(|ext| {
        caps.header_extensions
            .iter()
            .find(|cap_ext| cap_ext.preferred_id == ext.id && cap_ext.uri == ext.uri)
            .is_some()
    });

    // Reduce codecs' RTCP feedback. Use Transport-CC if available, REMB otherwise.
    if consumer_params
        .header_extensions
        .iter()
        .find(|ext| ext.uri.as_str() == TWCC_HEADER)
        .is_some()
    {
        for codec in consumer_params.codecs.iter_mut() {
            codec
                .rtcp_feedback_mut()
                .retain(|fb| fb.r#type != "goog-remb");
        }
    } else if consumer_params
        .header_extensions
        .iter()
        .find(|ext| ext.uri.as_str() == ABS_SEND_TIME_HEADER)
        .is_some()
    {
        for codec in consumer_params.codecs.iter_mut() {
            codec
                .rtcp_feedback_mut()
                .retain(|fb| fb.r#type != "transport-cc");
        }
    } else {
        for codec in consumer_params.codecs.iter_mut() {
            codec
                .rtcp_feedback_mut()
                .retain(|fb| fb.r#type != "transport-cc" && fb.r#type != "goog-remb");
        }
    }

    let mut consumer_encoding = RtpEncodingParameters {
        ssrc: Some(generate_ssrc()),
        ..RtpEncodingParameters::default()
    };

    if rtx_supported {
        consumer_encoding.rtx = Some(RtpEncodingParametersRtx {
            ssrc: generate_ssrc(),
        });
    }

    // If any of the consumable_params.encodings has scalability_mode, process it
    // (assume all encodings have the same value).
    let mut scalability_mode = consumable_params
        .encodings
        .iter()
        .find_map(|encoding| encoding.scalability_mode.clone());

    // If there is simulast, mangle spatial layers in scalabilityMode.
    if consumable_params.encodings.len() > 1 {
        scalability_mode = Some(format!(
            "S{}T{}",
            consumable_params.encodings.len(),
            scalability_mode
                .as_ref()
                .map(String::as_str)
                .map(scalability_modes::parse)
                .unwrap_or_default()
                .temporal_layers
        ));
    }

    consumer_encoding.scalability_mode = scalability_mode;

    // Use the maximum max_bitrate in any encoding and honor it in the Consumer's encoding.
    consumer_encoding.max_bitrate = consumable_params
        .encodings
        .iter()
        .map(|encoding| encoding.max_bitrate)
        .max()
        .flatten();

    // Set a single encoding for the Consumer.
    consumer_params.encodings.push(consumer_encoding);

    // Copy verbatim.
    consumer_params.rtcp = consumable_params.rtcp.clone();

    Ok(consumer_params)
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

impl<'a> From<&'a RtpCodecCapabilityFinalized> for CodecToMatch<'a> {
    fn from(rtp_codec_capability: &'a RtpCodecCapabilityFinalized) -> Self {
        match rtp_codec_capability {
            RtpCodecCapabilityFinalized::Audio {
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
            RtpCodecCapabilityFinalized::Video {
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
