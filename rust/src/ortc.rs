use std::collections::HashMap;

/// Media kind
#[derive(Debug)]
pub enum MediaKind {
    Audio,
    Video,
}

// TODO: supportedRtpCapabilities.ts file and generally update TypeScript references
/// Provides information on RTCP feedback messages for a specific codec. Those messages can be
/// transport layer feedback messages or codec-specific feedback messages. The list of RTCP
/// feedbacks supported by mediasoup is defined in the supportedRtpCapabilities.ts file.
#[derive(Debug)]
pub struct RtcpFeedback {
    // TODO: Enum?
    /// RTCP feedback type.
    pub r#type: String,
    /// RTCP feedback parameter.
    pub parameter: Option<String>,
}

// TODO: supportedRtpCapabilities.ts file and generally update TypeScript references
/// Provides information on the capabilities of a codec within the RTP capabilities. The list of
/// media codecs supported by mediasoup and their settings is defined in the
/// supportedRtpCapabilities.ts file.
///
/// Exactly one RtpCodecCapability will be present for each supported combination of parameters that
/// requires a distinct value of preferredPayloadType. For example:
///
/// - Multiple H264 codecs, each with their own distinct 'packetization-mode' and 'profile-level-id'
///   values.
/// - Multiple VP9 codecs, each with their own distinct 'profile-id' value.
///
/// RtpCodecCapability entries in the mediaCodecs array of RouterOptions do not require
/// preferredPayloadType field (if unset, mediasoup will choose a random one). If given, make sure
/// it's in the 96-127 range.
#[derive(Debug)]
pub struct RtpCodecCapability {
    /// Media kind
    pub kind: MediaKind,
    // TODO: Enum?
    /// The codec MIME media type/subtype (e.g. 'audio/opus', 'video/VP8').
    pub mime_type: String,
    /// The preferred RTP payload type.
    pub preferred_payload_type: Option<u32>,
    /// Codec clock rate expressed in Hertz.
    pub clock_rate: u32,
    /// The number of channels supported (e.g. two for stereo). Just for audio.
    /// Default 1.
    pub channels: Option<u8>,
    // TODO: Not sure if this hashmap is a correct type
    /// Codec specific parameters. Some parameters (such as 'packetization-mode' and
    /// 'profile-level-id' in H264 or 'profile-id' in VP9) are critical for codec matching.
    pub parameters: HashMap<String, String>,
    // TODO: Does this need to be optional or can be an empty vec?
    /// Transport layer and codec-specific feedback messages for this codec.
    pub rtcp_feedback: Option<Vec<RtcpFeedback>>,
}

/// Direction of RTP header extension.
#[derive(Debug)]
pub enum RtpHeaderExtensionDirection {
    // TODO: Serialization of all of these variants should be lowercase if we ever need it
    SendRecv,
    SendOnly,
    RecvOnly,
    Inactive,
}

// TODO: supportedRtpCapabilities.ts file and generally update TypeScript references
/// Provides information relating to supported header extensions. The list of RTP header extensions
/// supported by mediasoup is defined in the supportedRtpCapabilities.ts file.
///
/// mediasoup does not currently support encrypted RTP header extensions. The direction field is
/// just present in mediasoup RTP capabilities (retrieved via router.rtpCapabilities or
/// mediasoup.getSupportedRtpCapabilities()). It's ignored if present in endpoints' RTP
/// capabilities.
#[derive(Debug)]
pub struct RtpHeaderExtension {
    // TODO: TypeScript version makes this field both optional and possible to set to "",
    //  check if "" is actually needed
    /// Media kind. If `None`, it's valid for all kinds.
    /// Default any media kind.
    pub kind: Option<MediaKind>,
    /// The URI of the RTP header extension, as defined in RFC 5285.
    pub uri: String,
    /// The preferred numeric identifier that goes in the RTP packet. Must be unique.
    pub preferred_id: u16,
    /// If true, it is preferred that the value in the header be encrypted as per RFC 6904.
    /// Default false.
    pub preferred_encrypt: bool,
    /// If 'sendrecv', mediasoup supports sending and receiving this RTP extension. 'sendonly' means
    /// that mediasoup can send (but not receive) it. 'recvonly' means that mediasoup can receive
    /// (but not send) it.
    pub direction: RtpHeaderExtensionDirection,
}

/// The RTP capabilities define what mediasoup or an endpoint can receive at media level.
#[derive(Debug, Default)]
pub struct RtpCapabilities {
    // TODO: Does this need to be optional or can be an empty vec?
    /// Supported media and RTX codecs.
    pub codecs: Option<Vec<RtpCodecCapability>>,
    // TODO: Does this need to be optional or can be an empty vec?
    /// Supported RTP header extensions.
    pub header_extensions: Option<Vec<RtpHeaderExtension>>,
    // TODO: Does this need to be optional or can be an empty vec?
    // TODO: Enum instead of string?
    /// Supported FEC mechanisms.
    pub fec_mechanisms: Option<Vec<String>>,
}
