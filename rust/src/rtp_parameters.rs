//! Collection of RTP-related data structures that are used to specify codec parameters and
//! capabilities of various endpoints.

#[cfg(test)]
mod tests;

use crate::scalability_modes::ScalabilityMode;
use serde::de::{MapAccess, Visitor};
use serde::ser::SerializeStruct;
use serde::{de, Deserialize, Deserializer, Serialize, Serializer};
use std::borrow::Cow;
use std::collections::BTreeMap;
use std::fmt;
use std::iter::FromIterator;
use std::num::{NonZeroU32, NonZeroU8};

/// Codec specific parameters. Some parameters (such as `packetization-mode` and `profile-level-id`
/// in H264 or `profile-id` in VP9) are critical for codec matching.
#[derive(Debug, Default, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct RtpCodecParametersParameters(
    BTreeMap<Cow<'static, str>, RtpCodecParametersParametersValue>,
);

impl RtpCodecParametersParameters {
    /// Insert another parameter into collection.
    pub fn insert<K, V>(&mut self, key: K, value: V) -> &mut Self
    where
        K: Into<Cow<'static, str>>,
        V: Into<RtpCodecParametersParametersValue>,
    {
        self.0.insert(key.into(), value.into());
        self
    }

    /// Iterate over parameters in collection.
    pub fn iter(
        &self,
    ) -> std::collections::btree_map::Iter<'_, Cow<'static, str>, RtpCodecParametersParametersValue>
    {
        self.0.iter()
    }

    /// Get specific parameter from collection.
    #[must_use]
    pub fn get(&self, key: &str) -> Option<&RtpCodecParametersParametersValue> {
        self.0.get(key)
    }
}

impl<K, const N: usize> From<[(K, RtpCodecParametersParametersValue); N]>
    for RtpCodecParametersParameters
where
    K: Into<Cow<'static, str>>,
{
    fn from(array: [(K, RtpCodecParametersParametersValue); N]) -> Self {
        IntoIterator::into_iter(array).collect()
    }
}

impl IntoIterator for RtpCodecParametersParameters {
    type Item = (Cow<'static, str>, RtpCodecParametersParametersValue);
    type IntoIter =
        std::collections::btree_map::IntoIter<Cow<'static, str>, RtpCodecParametersParametersValue>;

    fn into_iter(
        self,
    ) -> std::collections::btree_map::IntoIter<Cow<'static, str>, RtpCodecParametersParametersValue>
    {
        self.0.into_iter()
    }
}

impl<K> Extend<(K, RtpCodecParametersParametersValue)> for RtpCodecParametersParameters
where
    K: Into<Cow<'static, str>>,
{
    fn extend<T: IntoIterator<Item = (K, RtpCodecParametersParametersValue)>>(&mut self, iter: T) {
        iter.into_iter().for_each(|(k, v)| {
            self.insert(k, v);
        });
    }
}

impl<K> FromIterator<(K, RtpCodecParametersParametersValue)> for RtpCodecParametersParameters
where
    K: Into<Cow<'static, str>>,
{
    fn from_iter<T: IntoIterator<Item = (K, RtpCodecParametersParametersValue)>>(iter: T) -> Self {
        Self(iter.into_iter().map(|(k, v)| (k.into(), v)).collect())
    }
}

/// Provides information on the capabilities of a codec within the RTP capabilities. The list of
/// media codecs supported by mediasoup and their settings is defined in the
/// `supported_rtp_capabilities.rs` file.
///
/// Exactly one [`RtpCodecCapabilityFinalized`] will be present for each supported combination of
/// parameters that requires a distinct value of `preferred_payload_type`. For example:
///
/// - Multiple H264 codecs, each with their own distinct `packetization-mode` and `profile-level-id`
///   values.
/// - Multiple VP9 codecs, each with their own distinct `profile-id` value.
///
/// This is similar to [`RtpCodecCapability`], but with `preferred_payload_type` field being
/// required.
#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(tag = "kind", rename_all = "lowercase")]
#[non_exhaustive]
pub enum RtpCodecCapabilityFinalized {
    /// Audio codec
    #[serde(rename_all = "camelCase")]
    Audio {
        /// The codec MIME media type/subtype (e.g. 'audio/opus').
        mime_type: MimeTypeAudio,
        /// The preferred RTP payload type.
        preferred_payload_type: u8,
        /// Codec clock rate expressed in Hertz.
        clock_rate: NonZeroU32,
        /// The number of channels supported (e.g. two for stereo). Just for audio.
        /// Default 1.
        channels: NonZeroU8,
        /// Codec specific parameters. Some parameters (such as `packetization-mode` and
        /// `profile-level-id` in H264 or `profile-id` in VP9) are critical for codec matching.
        parameters: RtpCodecParametersParameters,
        /// Transport layer and codec-specific feedback messages for this codec.
        rtcp_feedback: Vec<RtcpFeedback>,
    },
    /// Video codec
    #[serde(rename_all = "camelCase")]
    Video {
        /// The codec MIME media type/subtype (e.g. 'video/VP8').
        mime_type: MimeTypeVideo,
        /// The preferred RTP payload type.
        preferred_payload_type: u8,
        /// Codec clock rate expressed in Hertz.
        clock_rate: NonZeroU32,
        /// Codec specific parameters. Some parameters (such as `packetization-mode` and
        /// `profile-level-id` in H264 or `profile-id` in VP9) are critical for codec matching.
        parameters: RtpCodecParametersParameters,
        /// Transport layer and codec-specific feedback messages for this codec.
        rtcp_feedback: Vec<RtcpFeedback>,
    },
}

impl RtpCodecCapabilityFinalized {
    pub(crate) fn is_rtx(&self) -> bool {
        match self {
            Self::Audio { mime_type, .. } => mime_type == &MimeTypeAudio::Rtx,
            Self::Video { mime_type, .. } => mime_type == &MimeTypeVideo::Rtx,
        }
    }

    pub(crate) fn clock_rate(&self) -> NonZeroU32 {
        let (Self::Audio { clock_rate, .. } | Self::Video { clock_rate, .. }) = self;
        *clock_rate
    }

    pub(crate) fn parameters(&self) -> &RtpCodecParametersParameters {
        let (Self::Audio { parameters, .. } | Self::Video { parameters, .. }) = self;
        parameters
    }

    pub(crate) fn parameters_mut(&mut self) -> &mut RtpCodecParametersParameters {
        let (Self::Audio { parameters, .. } | Self::Video { parameters, .. }) = self;
        parameters
    }

    pub(crate) fn preferred_payload_type(&self) -> u8 {
        match self {
            Self::Audio {
                preferred_payload_type,
                ..
            }
            | Self::Video {
                preferred_payload_type,
                ..
            } => *preferred_payload_type,
        }
    }
}

/// The RTP capabilities define what mediasoup or an endpoint can receive at media level.
#[derive(Debug, Default, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
pub struct RtpCapabilitiesFinalized {
    /// Supported media and RTX codecs.
    pub codecs: Vec<RtpCodecCapabilityFinalized>,
    /// Supported RTP header extensions.
    pub header_extensions: Vec<RtpHeaderExtension>,
}

/// Media kind
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum MediaKind {
    /// Audio
    Audio,
    /// Video
    Video,
}

/// Known Audio or Video MIME type.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(untagged)]
pub enum MimeType {
    /// Audio
    Audio(MimeTypeAudio),
    /// Video
    Video(MimeTypeVideo),
}

/// Known Audio MIME types.
#[allow(non_camel_case_types)]
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub enum MimeTypeAudio {
    /// Opus
    #[serde(rename = "audio/opus")]
    Opus,
    /// Multi-channel Opus (Surround sound in Chromium)
    #[serde(rename = "audio/multiopus")]
    MultiChannelOpus,
    /// PCMU
    #[serde(rename = "audio/PCMU")]
    Pcmu,
    /// PCMA
    #[serde(rename = "audio/PCMA")]
    Pcma,
    /// ISAC
    #[serde(rename = "audio/ISAC")]
    Isac,
    /// G722
    #[serde(rename = "audio/G722")]
    G722,
    /// iLBC
    #[serde(rename = "audio/iLBC")]
    Ilbc,
    /// SILK
    #[serde(rename = "audio/SILK")]
    Silk,
    /// CN
    #[serde(rename = "audio/CN")]
    Cn,
    /// TelephoneEvent
    #[serde(rename = "audio/telephone-event")]
    TelephoneEvent,
    /// RTX
    #[serde(rename = "audio/rtx")]
    Rtx,
    /// RED
    #[serde(rename = "audio/red")]
    Red,
}

/// Known Video MIME types.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub enum MimeTypeVideo {
    /// VP8
    #[serde(rename = "video/VP8")]
    Vp8,
    /// VP9
    #[serde(rename = "video/VP9")]
    Vp9,
    /// H264
    #[serde(rename = "video/H264")]
    H264,
    /// H264-SVC
    #[serde(rename = "video/H264-SVC")]
    H264Svc,
    /// H265
    #[serde(rename = "video/H265")]
    H265,
    /// RTX
    #[serde(rename = "video/rtx")]
    Rtx,
    /// RED
    #[serde(rename = "video/red")]
    Red,
    /// ULPFEC
    #[serde(rename = "video/ulpfec")]
    Ulpfec,
}

/// Provides information on the capabilities of a codec within the RTP capabilities. The list of
/// media codecs supported by mediasoup and their settings is defined in the
/// `supported_rtp_capabilities.rs` file.
///
/// Exactly one [`RtpCodecCapability`] will be present for each supported combination of parameters
/// that requires a distinct value of `preferred_payload_type`. For example:
///
/// - Multiple H264 codecs, each with their own distinct `packetization-mode` and `profile-level-id`
///   values.
/// - Multiple VP9 codecs, each with their own distinct `profile-id` value.
///
/// [`RtpCodecCapability`] entries in the `media_codecs` vector of
/// [`RouterOptions`](crate::router::RouterOptions) do not require `preferred_payload_type` field
/// (if unset, mediasoup will choose a random one). If given, make sure it's in the 96-127 range.
#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(tag = "kind", rename_all = "lowercase")]
pub enum RtpCodecCapability {
    /// Audio codec capability
    #[serde(rename_all = "camelCase")]
    Audio {
        /// The codec MIME media type/subtype (e.g. 'audio/opus').
        mime_type: MimeTypeAudio,
        /// The preferred RTP payload type.
        #[serde(skip_serializing_if = "Option::is_none")]
        preferred_payload_type: Option<u8>,
        /// Codec clock rate expressed in Hertz.
        clock_rate: NonZeroU32,
        /// The number of channels supported (e.g. two for stereo). Just for audio.
        /// Default 1.
        channels: NonZeroU8,
        /// Codec specific parameters. Some parameters (such as `packetization-mode` and
        /// `profile-level-id` in H264 or `profile-id` in VP9) are critical for codec matching.
        parameters: RtpCodecParametersParameters,
        /// Transport layer and codec-specific feedback messages for this codec.
        rtcp_feedback: Vec<RtcpFeedback>,
    },
    /// Video codec capability
    #[serde(rename_all = "camelCase")]
    Video {
        /// The codec MIME media type/subtype (e.g. 'video/VP8').
        mime_type: MimeTypeVideo,
        /// The preferred RTP payload type.
        #[serde(skip_serializing_if = "Option::is_none")]
        preferred_payload_type: Option<u8>,
        /// Codec clock rate expressed in Hertz.
        clock_rate: NonZeroU32,
        /// Codec specific parameters. Some parameters (such as `packetization-mode` and
        /// `profile-level-id` in H264 or `profile-id` in VP9) are critical for codec matching.
        parameters: RtpCodecParametersParameters,
        /// Transport layer and codec-specific feedback messages for this codec.
        rtcp_feedback: Vec<RtcpFeedback>,
    },
}

impl RtpCodecCapability {
    pub(crate) fn mime_type(&self) -> MimeType {
        match self {
            Self::Audio { mime_type, .. } => MimeType::Audio(*mime_type),
            Self::Video { mime_type, .. } => MimeType::Video(*mime_type),
        }
    }

    pub(crate) fn parameters(&self) -> &RtpCodecParametersParameters {
        let (Self::Audio { parameters, .. } | Self::Video { parameters, .. }) = self;
        parameters
    }

    pub(crate) fn parameters_mut(&mut self) -> &mut RtpCodecParametersParameters {
        let (Self::Audio { parameters, .. } | Self::Video { parameters, .. }) = self;
        parameters
    }

    pub(crate) fn preferred_payload_type(&self) -> Option<u8> {
        match self {
            Self::Audio {
                preferred_payload_type,
                ..
            }
            | Self::Video {
                preferred_payload_type,
                ..
            } => *preferred_payload_type,
        }
    }

    pub(crate) fn rtcp_feedback(&self) -> &Vec<RtcpFeedback> {
        let (Self::Audio { rtcp_feedback, .. } | Self::Video { rtcp_feedback, .. }) = self;
        rtcp_feedback
    }
}

/// The RTP capabilities define what mediasoup or an endpoint can receive at media level.
#[derive(Debug, Default, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RtpCapabilities {
    /// Supported media and RTX codecs.
    pub codecs: Vec<RtpCodecCapability>,
    /// Supported RTP header extensions.
    pub header_extensions: Vec<RtpHeaderExtension>,
}

/// Direction of RTP header extension.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum RtpHeaderExtensionDirection {
    /// SendRecv
    SendRecv,
    /// SendOnly
    SendOnly,
    /// RecvOnly
    RecvOnly,
    /// Inactive
    Inactive,
}

impl Default for RtpHeaderExtensionDirection {
    fn default() -> Self {
        Self::SendRecv
    }
}

/// URI for supported RTP header extension
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub enum RtpHeaderExtensionUri {
    /// urn:ietf:params:rtp-hdrext:sdes:mid
    #[serde(rename = "urn:ietf:params:rtp-hdrext:sdes:mid")]
    Mid,
    /// urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id
    #[serde(rename = "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id")]
    RtpStreamId,
    /// urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id
    #[serde(rename = "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id")]
    RepairRtpStreamId,
    /// <http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07>
    #[serde(rename = "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07")]
    FrameMarkingDraft07,
    /// urn:ietf:params:rtp-hdrext:framemarking
    #[serde(rename = "urn:ietf:params:rtp-hdrext:framemarking")]
    FrameMarking,
    /// urn:ietf:params:rtp-hdrext:ssrc-audio-level
    #[serde(rename = "urn:ietf:params:rtp-hdrext:ssrc-audio-level")]
    AudioLevel,
    /// urn:3gpp:video-orientation
    #[serde(rename = "urn:3gpp:video-orientation")]
    VideoOrientation,
    /// urn:ietf:params:rtp-hdrext:toffset
    #[serde(rename = "urn:ietf:params:rtp-hdrext:toffset")]
    TimeOffset,
    /// <http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01>
    #[serde(rename = "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01")]
    TransportWideCcDraft01,
    /// <http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time>
    #[serde(rename = "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time")]
    AbsSendTime,
    /// <http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time>
    #[serde(rename = "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time")]
    AbsCaptureTime,
    #[doc(hidden)]
    #[serde(other, rename = "unsupported")]
    Unsupported,
}

impl RtpHeaderExtensionUri {
    /// RTP header extension as a string
    #[must_use]
    pub fn as_str(self) -> &'static str {
        match self {
            RtpHeaderExtensionUri::Mid => "urn:ietf:params:rtp-hdrext:sdes:mid",
            RtpHeaderExtensionUri::RtpStreamId => "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id",
            RtpHeaderExtensionUri::RepairRtpStreamId => {
                "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id"
            }
            RtpHeaderExtensionUri::FrameMarkingDraft07 => {
                "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07"
            }
            RtpHeaderExtensionUri::FrameMarking => "urn:ietf:params:rtp-hdrext:framemarking",
            RtpHeaderExtensionUri::AudioLevel => "urn:ietf:params:rtp-hdrext:ssrc-audio-level",
            RtpHeaderExtensionUri::VideoOrientation => "urn:3gpp:video-orientation",
            RtpHeaderExtensionUri::TimeOffset => "urn:ietf:params:rtp-hdrext:toffset",
            RtpHeaderExtensionUri::TransportWideCcDraft01 => {
                "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"
            }
            RtpHeaderExtensionUri::AbsSendTime => {
                "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time"
            }
            RtpHeaderExtensionUri::AbsCaptureTime => {
                "http://www.webrtc.org/experiments/rtp-hdrext/abs-capture-time"
            }
            RtpHeaderExtensionUri::Unsupported => "unsupported",
        }
    }
}

/// Provides information relating to supported header extensions. The list of RTP header extensions
/// supported by mediasoup is defined in the `supported_rtp_capabilities.rs` file.
///
/// mediasoup does not currently support encrypted RTP header extensions. The direction field is
/// just present in mediasoup RTP capabilities (retrieved via
/// `mediasoup::router::Router::rtp_capabilities()` or
/// `mediasoup::supported_rtp_capabilities::get_supported_rtp_capabilities()`. It's ignored if
/// present in endpoints' RTP capabilities.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RtpHeaderExtension {
    /// Media kind.
    pub kind: MediaKind,
    /// The URI of the RTP header extension, as defined in RFC 5285.
    pub uri: RtpHeaderExtensionUri,
    /// The preferred numeric identifier that goes in the RTP packet. Must be unique.
    pub preferred_id: u16,
    /// If true, it is preferred that the value in the header be encrypted as per RFC 6904.
    /// Default false.
    pub preferred_encrypt: bool,
    /// If `SendRecv`, mediasoup supports sending and receiving this RTP extension. `SendOnly` means
    /// that mediasoup can send (but not receive) it. `RecvOnly` means that mediasoup can receive
    /// (but not send) it.
    pub direction: RtpHeaderExtensionDirection,
}

/// The RTP send parameters describe a media stream received by mediasoup from
/// an endpoint through its corresponding mediasoup Producer. These parameters
/// may include a mid value that the mediasoup transport will use to match
/// received RTP packets based on their MID RTP extension value.
///
/// mediasoup allows RTP send parameters with a single encoding and with multiple
/// encodings (simulcast). In the latter case, each entry in the encodings array
/// must include a ssrc field or a rid field (the RID RTP extension value). Check
/// the Simulcast and SVC sections for more information.
///
/// The RTP receive parameters describe a media stream as sent by mediasoup to
/// an endpoint through its corresponding mediasoup Consumer. The mid value is
/// unset (mediasoup does not include the MID RTP extension into RTP packets
/// being sent to endpoints).
///
/// There is a single entry in the encodings array (even if the corresponding
/// producer uses simulcast). The consumer sends a single and continuous RTP
/// stream to the endpoint and spatial/temporal layer selection is possible via
/// consumer.setPreferredLayers().
///
/// As an exception, previous bullet is not true when consuming a stream over a
/// [`PipeTransport`](crate::pipe_transport::PipeTransport), in which all RTP streams from the
/// associated producer are forwarded verbatim through the consumer.
///
/// The RTP receive parameters will always have their ssrc values randomly
/// generated for all of its  encodings (and optional rtx: { ssrc: XXXX } if the
/// endpoint supports RTX), regardless of the original RTP send parameters in
/// the associated producer. This applies even if the producer's encodings have
/// rid set.
#[derive(Debug, Default, Clone, PartialEq, PartialOrd, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RtpParameters {
    /// The MID RTP extension value as defined in the BUNDLE specification.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub mid: Option<String>,
    /// Media and RTX codecs in use.
    pub codecs: Vec<RtpCodecParameters>,
    /// RTP header extensions in use.
    pub header_extensions: Vec<RtpHeaderExtensionParameters>,
    /// Transmitted RTP streams and their settings.
    pub encodings: Vec<RtpEncodingParameters>,
    /// Parameters used for RTCP.
    pub rtcp: RtcpParameters,
}

/// Single value used in RTP codec parameters.
#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(untagged)]
pub enum RtpCodecParametersParametersValue {
    /// String value
    String(Cow<'static, str>),
    /// Numerical value
    Number(u32),
}

impl From<Cow<'static, str>> for RtpCodecParametersParametersValue {
    fn from(s: Cow<'static, str>) -> Self {
        Self::String(s)
    }
}

impl From<String> for RtpCodecParametersParametersValue {
    fn from(s: String) -> Self {
        Self::String(s.into())
    }
}

impl From<&'static str> for RtpCodecParametersParametersValue {
    fn from(s: &'static str) -> Self {
        Self::String(s.into())
    }
}

impl From<u8> for RtpCodecParametersParametersValue {
    fn from(n: u8) -> Self {
        Self::Number(u32::from(n))
    }
}

impl From<u16> for RtpCodecParametersParametersValue {
    fn from(n: u16) -> Self {
        Self::Number(u32::from(n))
    }
}

impl From<u32> for RtpCodecParametersParametersValue {
    fn from(n: u32) -> Self {
        Self::Number(n)
    }
}

/// Provides information on codec settings within the RTP parameters. The list
/// of media codecs supported by mediasoup and their settings is defined in the
/// `supported_rtp_capabilities.rs` file.
#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(untagged, rename_all = "lowercase")]
pub enum RtpCodecParameters {
    /// Audio codec
    #[serde(rename_all = "camelCase")]
    Audio {
        /// The codec MIME media type/subtype (e.g. `audio/opus`).
        mime_type: MimeTypeAudio,
        /// The value that goes in the RTP Payload Type Field. Must be unique.
        payload_type: u8,
        /// Codec clock rate expressed in Hertz.
        clock_rate: NonZeroU32,
        /// The number of channels supported (e.g. two for stereo).
        /// Default 1.
        channels: NonZeroU8,
        /// Codec-specific parameters available for signaling. Some parameters (such as
        /// `packetization-mode` and `profile-level-id` in H264 or `profile-id` in VP9) are critical for
        /// codec matching.
        parameters: RtpCodecParametersParameters,
        /// Transport layer and codec-specific feedback messages for this codec.
        rtcp_feedback: Vec<RtcpFeedback>,
    },
    /// Video codec
    #[serde(rename_all = "camelCase")]
    Video {
        /// The codec MIME media type/subtype (e.g. `video/VP8`).
        mime_type: MimeTypeVideo,
        /// The value that goes in the RTP Payload Type Field. Must be unique.
        payload_type: u8,
        /// Codec clock rate expressed in Hertz.
        clock_rate: NonZeroU32,
        /// Codec-specific parameters available for signaling. Some parameters (such as
        /// `packetization-mode` and `profile-level-id` in H264 or `profile-id` in VP9) are critical for
        /// codec matching.
        parameters: RtpCodecParametersParameters,
        /// Transport layer and codec-specific feedback messages for this codec.
        rtcp_feedback: Vec<RtcpFeedback>,
    },
}

impl RtpCodecParameters {
    pub(crate) fn is_rtx(&self) -> bool {
        match self {
            Self::Audio { mime_type, .. } => mime_type == &MimeTypeAudio::Rtx,
            Self::Video { mime_type, .. } => mime_type == &MimeTypeVideo::Rtx,
        }
    }

    pub(crate) fn mime_type(&self) -> MimeType {
        match self {
            Self::Audio { mime_type, .. } => MimeType::Audio(*mime_type),
            Self::Video { mime_type, .. } => MimeType::Video(*mime_type),
        }
    }

    pub(crate) fn payload_type(&self) -> u8 {
        let (Self::Audio { payload_type, .. } | Self::Video { payload_type, .. }) = self;
        *payload_type
    }

    pub(crate) fn parameters(&self) -> &RtpCodecParametersParameters {
        let (Self::Audio { parameters, .. } | Self::Video { parameters, .. }) = self;
        parameters
    }

    pub(crate) fn rtcp_feedback_mut(&mut self) -> &mut Vec<RtcpFeedback> {
        let (Self::Audio { rtcp_feedback, .. } | Self::Video { rtcp_feedback, .. }) = self;
        rtcp_feedback
    }
}

/// Provides information on RTCP feedback messages for a specific codec. Those messages can be
/// transport layer feedback messages or codec-specific feedback messages. The list of RTCP
/// feedbacks supported by mediasoup is defined in the `supported_rtp_capabilities.rs` file.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub enum RtcpFeedback {
    /// NACK
    Nack,
    /// NACK PLI
    NackPli,
    /// CCM FIR
    CcmFir,
    /// goog-remb
    GoogRemb,
    /// transport-cc
    TransportCc,
    #[doc(hidden)]
    Unsupported,
}

impl Serialize for RtcpFeedback {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let mut rtcp_feedback = serializer.serialize_struct("RtcpFeedback", 2)?;
        match self {
            RtcpFeedback::Nack => {
                rtcp_feedback.serialize_field("type", "nack")?;
                rtcp_feedback.serialize_field("parameter", "")?;
            }
            RtcpFeedback::NackPli => {
                rtcp_feedback.serialize_field("type", "nack")?;
                rtcp_feedback.serialize_field("parameter", "pli")?;
            }
            RtcpFeedback::CcmFir => {
                rtcp_feedback.serialize_field("type", "ccm")?;
                rtcp_feedback.serialize_field("parameter", "fir")?;
            }
            RtcpFeedback::GoogRemb => {
                rtcp_feedback.serialize_field("type", "goog-remb")?;
                rtcp_feedback.serialize_field("parameter", "")?;
            }
            RtcpFeedback::TransportCc => {
                rtcp_feedback.serialize_field("type", "transport-cc")?;
                rtcp_feedback.serialize_field("parameter", "")?;
            }
            RtcpFeedback::Unsupported => {
                rtcp_feedback.serialize_field("type", "unknown")?;
                rtcp_feedback.serialize_field("parameter", "")?;
            }
        }
        rtcp_feedback.end()
    }
}

impl<'de> Deserialize<'de> for RtcpFeedback {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        #[derive(Deserialize)]
        #[serde(field_identifier, rename_all = "lowercase")]
        enum Field {
            Type,
            Parameter,
        }

        struct RtcpFeedbackVisitor;

        impl<'de> Visitor<'de> for RtcpFeedbackVisitor {
            type Value = RtcpFeedback;

            fn expecting(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
                formatter.write_str(
                    r#"RTCP feedback type and parameter like {"type": "nack", "parameter": ""}"#,
                )
            }

            fn visit_map<V>(self, mut map: V) -> Result<Self::Value, V::Error>
            where
                V: MapAccess<'de>,
            {
                let mut r#type = None::<Cow<'_, str>>;
                let mut parameter = Cow::Borrowed("");
                while let Some(key) = map.next_key()? {
                    match key {
                        Field::Type => {
                            if r#type.is_some() {
                                return Err(de::Error::duplicate_field("type"));
                            }
                            r#type = Some(map.next_value()?);
                        }
                        Field::Parameter => {
                            if parameter != "" {
                                return Err(de::Error::duplicate_field("parameter"));
                            }
                            parameter = map.next_value()?;
                        }
                    }
                }
                let r#type = r#type.ok_or_else(|| de::Error::missing_field("type"))?;

                Ok(match (r#type.as_ref(), parameter.as_ref()) {
                    ("nack", "") => RtcpFeedback::Nack,
                    ("nack", "pli") => RtcpFeedback::NackPli,
                    ("ccm", "fir") => RtcpFeedback::CcmFir,
                    ("goog-remb", "") => RtcpFeedback::GoogRemb,
                    ("transport-cc", "") => RtcpFeedback::TransportCc,
                    _ => RtcpFeedback::Unsupported,
                })
            }
        }

        const FIELDS: &[&str] = &["type", "parameter"];
        deserializer.deserialize_struct("RtcpFeedback", FIELDS, RtcpFeedbackVisitor)
    }
}

/// RTX stream information. It must contain a numeric ssrc field indicating the RTX SSRC.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct RtpEncodingParametersRtx {
    /// The media SSRC.
    pub ssrc: u32,
}

/// Provides information relating to an encoding, which represents a media RTP
/// stream and its associated RTX stream (if any).
#[derive(Debug, Default, Clone, PartialEq, PartialOrd, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RtpEncodingParameters {
    /// The media SSRC.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub ssrc: Option<u32>,
    /// The RID RTP extension value. Must be unique.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub rid: Option<String>,
    /// Codec payload type this encoding affects. If unset, first media codec is chosen.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub codec_payload_type: Option<u8>,
    /// RTX stream information. It must contain a numeric ssrc field indicating the RTX SSRC.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub rtx: Option<RtpEncodingParametersRtx>,
    /// It indicates whether discontinuous RTP transmission will be used. Useful for audio (if the
    /// codec supports it) and for video screen sharing (when static content is being transmitted,
    /// this option disables the RTP inactivity checks in mediasoup).
    /// Default false.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub dtx: Option<bool>,
    /// Number of spatial and temporal layers in the RTP stream.
    #[serde(default, skip_serializing_if = "ScalabilityMode::is_none")]
    pub scalability_mode: ScalabilityMode,
    /// Factor by which to reduce the size of a video track during encoding.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub scale_resolution_down_by: Option<f64>,
    /// Maximum number of bits per second to allow a track encoded with this encoding to use.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub max_bitrate: Option<u32>,
}

/// Defines a RTP header extension within the RTP parameters. The list of RTP
/// header extensions supported by mediasoup is defined in the `supported_rtp_capabilities.rs` file.
///
/// mediasoup does not currently support encrypted RTP header extensions and no
/// parameters are currently considered.
#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
pub struct RtpHeaderExtensionParameters {
    /// The URI of the RTP header extension, as defined in RFC 5285.
    pub uri: RtpHeaderExtensionUri,
    /// The numeric identifier that goes in the RTP packet. Must be unique.
    pub id: u16,
    /// If true, the value in the header is encrypted as per RFC 6904.
    /// Default false.
    pub encrypt: bool,
    // This field is not used by mediasoup currently
    // /// Configuration parameters for the header extension.
    // pub parameters: RtpCodecParametersParameters,
}

/// Provides information on RTCP settings within the RTP parameters.
///
/// If no cname is given in a producer's RTP parameters, the mediasoup transport will choose a
/// random one that will be used into RTCP SDES messages sent to all its associated consumers.
///
/// mediasoup assumes `reduced_size` to always be true.
#[derive(Debug, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct RtcpParameters {
    /// The Canonical Name (CNAME) used by RTCP (e.g. in SDES messages).
    #[serde(skip_serializing_if = "Option::is_none")]
    pub cname: Option<String>,
    /// Whether reduced size RTCP RFC 5506 is configured (if true) or compound RTCP
    /// as specified in RFC 3550 (if false). Default true.
    pub reduced_size: bool,
    /// Whether RTCP-mux is used. Default true.
    #[serde(skip_serializing_if = "Option::is_none")]
    pub mux: Option<bool>,
}

impl Default for RtcpParameters {
    fn default() -> Self {
        Self {
            cname: None,
            reduced_size: true,
            mux: None,
        }
    }
}
