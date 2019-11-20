"use strict";
var __importStar = (this && this.__importStar) || function (mod) {
    if (mod && mod.__esModule) return mod;
    var result = {};
    if (mod != null) for (var k in mod) if (Object.hasOwnProperty.call(mod, k)) result[k] = mod[k];
    result["default"] = mod;
    return result;
};
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const h264 = __importStar(require("h264-profile-level-id"));
const utils = __importStar(require("./utils"));
const errors_1 = require("./errors");
const supportedRtpCapabilities_1 = __importDefault(require("./supportedRtpCapabilities"));
const scalabilityModes_1 = require("./scalabilityModes");
const DynamicPayloadTypes = [
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
    111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122, 123, 124, 125, 126, 127, 96, 97, 98, 99
];
/**
 * Generate RTP capabilities for the Router based on the given media codecs and
 * mediasoup supported RTP capabilities.
 */
function generateRouterRtpCapabilities(mediaCodecs = []) {
    if (!Array.isArray(mediaCodecs))
        throw new TypeError('mediaCodecs must be an Array');
    const dynamicPayloadTypes = utils.clone(DynamicPayloadTypes);
    const supportedCodecs = supportedRtpCapabilities_1.default.codecs;
    const caps = {
        codecs: [],
        headerExtensions: supportedRtpCapabilities_1.default.headerExtensions,
        fecMechanisms: supportedRtpCapabilities_1.default.fecMechanisms
    };
    for (const mediaCodec of mediaCodecs) {
        assertCodecCapability(mediaCodec);
        const matchedSupportedCodec = supportedCodecs
            .find((supportedCodec) => (matchCodecs(mediaCodec, supportedCodec, { strict: false })));
        if (!matchedSupportedCodec) {
            throw new errors_1.UnsupportedError(`media codec not supported [mimeType:${mediaCodec.mimeType}]`);
        }
        // Clone the supported codec.
        const codec = utils.clone(matchedSupportedCodec);
        // If the given media codec has preferredPayloadType, keep it.
        if (typeof mediaCodec.preferredPayloadType === 'number') {
            codec.preferredPayloadType = mediaCodec.preferredPayloadType;
            // Also remove the pt from the list of available dynamic values.
            const idx = dynamicPayloadTypes.indexOf(codec.preferredPayloadType);
            if (idx > -1)
                dynamicPayloadTypes.splice(idx, 1);
        }
        // Otherwise if the supported codec has preferredPayloadType, use it.
        else if (typeof codec.preferredPayloadType === 'number') {
            // No need to remove it from the list since it's not a dynamic value.
        }
        // Otherwise choose a dynamic one.
        else {
            // Take the first available pt and remove it from the list.
            const pt = dynamicPayloadTypes.shift();
            if (!pt)
                throw new Error('cannot allocate more dynamic codec payload types');
            codec.preferredPayloadType = pt;
        }
        // Ensure there is not duplicated preferredPayloadType values.
        if (caps.codecs.some((c) => c.preferredPayloadType === codec.preferredPayloadType))
            throw new Error('duplicated codec.preferredPayloadType');
        // Normalize channels.
        if (codec.kind !== 'audio')
            delete codec.channels;
        else if (!codec.channels)
            codec.channels = 1;
        // Merge the media codec parameters.
        codec.parameters = Object.assign(Object.assign({}, codec.parameters), mediaCodec.parameters);
        // Make rtcpFeedback an array.
        codec.rtcpFeedback = codec.rtcpFeedback || [];
        // Append to the codec list.
        caps.codecs.push(codec);
        // Add a RTX video codec if video.
        if (codec.kind === 'video') {
            // Take the first available pt and remove it from the list.
            const pt = dynamicPayloadTypes.shift();
            if (!pt)
                throw new Error('cannot allocate more dynamic codec payload types');
            const rtxCodec = {
                kind: codec.kind,
                mimeType: `${codec.kind}/rtx`,
                preferredPayloadType: pt,
                clockRate: codec.clockRate,
                rtcpFeedback: [],
                parameters: {
                    apt: codec.preferredPayloadType
                }
            };
            // Append to the codec list.
            caps.codecs.push(rtxCodec);
        }
    }
    return caps;
}
exports.generateRouterRtpCapabilities = generateRouterRtpCapabilities;
/**
 * Get a mapping of codec payloads and encodings of the given Producer RTP
 * parameters as values expected by the Router.
 *
 * It may throw if invalid or non supported RTP parameters are given.
 */
function getProducerRtpParametersMapping(params, caps) {
    const rtpMapping = {
        codecs: [],
        encodings: []
    };
    // Match parameters media codecs to capabilities media codecs.
    const codecToCapCodec = new Map();
    for (const codec of params.codecs || []) {
        assertCodecParameters(codec);
        if (/.+\/rtx$/i.test(codec.mimeType))
            continue;
        // Search for the same media codec in capabilities.
        const matchedCapCodec = caps.codecs
            .find((capCodec) => (matchCodecs(codec, capCodec, { strict: true, modify: true })));
        if (!matchedCapCodec) {
            throw new errors_1.UnsupportedError(`unsupported codec [mimeType:${codec.mimeType}, payloadType:${codec.payloadType}]`);
        }
        codecToCapCodec.set(codec, matchedCapCodec);
    }
    // Match parameters RTX codecs to capabilities RTX codecs.
    for (const codec of params.codecs || []) {
        if (!/.+\/rtx$/i.test(codec.mimeType))
            continue;
        else if (typeof codec.parameters !== 'object')
            throw TypeError('missing parameters in RTX codec');
        // Search for the associated media codec.
        const associatedMediaCodec = params.codecs
            .find((mediaCodec) => mediaCodec.payloadType === codec.parameters.apt);
        if (!associatedMediaCodec) {
            throw new TypeError(`missing media codec found for RTX PT ${codec.payloadType}`);
        }
        const capMediaCodec = codecToCapCodec.get(associatedMediaCodec);
        // Ensure that the capabilities media codec has a RTX codec.
        const associatedCapRtxCodec = caps.codecs
            .find((capCodec) => (/.+\/rtx$/i.test(capCodec.mimeType) &&
            capCodec.parameters.apt === capMediaCodec.preferredPayloadType));
        if (!associatedCapRtxCodec) {
            throw new errors_1.UnsupportedError(`no RTX codec for capability codec PT ${capMediaCodec.preferredPayloadType}`);
        }
        codecToCapCodec.set(codec, associatedCapRtxCodec);
    }
    // Generate codecs mapping.
    for (const [codec, capCodec] of codecToCapCodec) {
        rtpMapping.codecs.push({
            payloadType: codec.payloadType,
            mappedPayloadType: capCodec.preferredPayloadType
        });
    }
    // Generate encodings mapping.
    let mappedSsrc = utils.generateRandomNumber();
    for (const encoding of (params.encodings || [])) {
        const mappedEncoding = {};
        mappedEncoding.mappedSsrc = mappedSsrc++;
        if (encoding.rid)
            mappedEncoding.rid = encoding.rid;
        if (encoding.ssrc)
            mappedEncoding.ssrc = encoding.ssrc;
        if (encoding.scalabilityMode)
            mappedEncoding.scalabilityMode = encoding.scalabilityMode;
        rtpMapping.encodings.push(mappedEncoding);
    }
    return rtpMapping;
}
exports.getProducerRtpParametersMapping = getProducerRtpParametersMapping;
/**
 * Generate RTP parameters to be internally used by Consumers given the RTP
 * parameters of a Producer and the RTP capabilities of the Router.
 */
function getConsumableRtpParameters(kind, params, caps, rtpMapping) {
    const consumableParams = {
        codecs: [],
        headerExtensions: [],
        encodings: [],
        rtcp: {}
    };
    for (const codec of params.codecs || []) {
        assertCodecParameters(codec);
        if (/.+\/rtx$/i.test(codec.mimeType))
            continue;
        const consumableCodecPt = rtpMapping.codecs
            .find((entry) => entry.payloadType === codec.payloadType)
            .mappedPayloadType;
        const matchedCapCodec = caps.codecs
            .find((capCodec) => capCodec.preferredPayloadType === consumableCodecPt);
        const consumableCodec = {
            mimeType: matchedCapCodec.mimeType,
            clockRate: matchedCapCodec.clockRate,
            payloadType: matchedCapCodec.preferredPayloadType,
            channels: matchedCapCodec.channels,
            rtcpFeedback: matchedCapCodec.rtcpFeedback,
            parameters: codec.parameters // Keep the Producer parameters.
        };
        if (!consumableCodec.channels)
            delete consumableCodec.channels;
        consumableParams.codecs.push(consumableCodec);
        const consumableCapRtxCodec = caps.codecs
            .find((capRtxCodec) => (/.+\/rtx$/i.test(capRtxCodec.mimeType) &&
            capRtxCodec.parameters.apt === consumableCodec.payloadType));
        if (consumableCapRtxCodec) {
            const consumableRtxCodec = {
                mimeType: consumableCapRtxCodec.mimeType,
                clockRate: consumableCapRtxCodec.clockRate,
                payloadType: consumableCapRtxCodec.preferredPayloadType,
                channels: consumableCapRtxCodec.channels,
                rtcpFeedback: consumableCapRtxCodec.rtcpFeedback,
                parameters: consumableCapRtxCodec.parameters
            };
            if (!consumableRtxCodec.channels)
                delete consumableRtxCodec.channels;
            consumableParams.codecs.push(consumableRtxCodec);
        }
    }
    for (const capExt of caps.headerExtensions) {
        // Just take RTP header extension that can be used in Consumers.
        if (capExt.kind !== kind ||
            (capExt.direction !== 'sendrecv' && capExt.direction !== 'sendonly')) {
            continue;
        }
        const consumableExt = {
            uri: capExt.uri,
            id: capExt.preferredId
        };
        consumableParams.headerExtensions.push(consumableExt);
    }
    // Clone Producer encodings since we'll mangle them.
    const consumableEncodings = utils.clone(params.encodings);
    for (let i = 0; i < consumableEncodings.length; ++i) {
        const consumableEncoding = consumableEncodings[i];
        const { mappedSsrc } = rtpMapping.encodings[i];
        // Remove useless fields.
        delete consumableEncoding.rid;
        delete consumableEncoding.rtx;
        delete consumableEncoding.codecPayloadType;
        // Set the mapped ssrc.
        consumableEncoding.ssrc = mappedSsrc;
        consumableParams.encodings.push(consumableEncoding);
    }
    consumableParams.rtcp =
        {
            cname: params.rtcp.cname,
            reducedSize: true,
            mux: true
        };
    return consumableParams;
}
exports.getConsumableRtpParameters = getConsumableRtpParameters;
/**
 * Check whether the given RTP capabilities can consume the given Producer.
 */
function canConsume(consumableParams, caps) {
    const matchingCodecs = [];
    for (const capCodec of caps.codecs || []) {
        assertCodecCapability(capCodec);
    }
    for (const codec of consumableParams.codecs) {
        const matchedCapCodec = caps.codecs
            .find((capCodec) => matchCodecs(capCodec, codec, { strict: true }));
        if (!matchedCapCodec)
            continue;
        matchingCodecs.push(codec);
    }
    // Ensure there is at least one media codec.
    if (matchingCodecs.length === 0 ||
        /.+\/rtx$/i.test(matchingCodecs[0].mimeType)) {
        return false;
    }
    return true;
}
exports.canConsume = canConsume;
/**
 * Generate RTP parameters for a specific Consumer.
 *
 * It reduces encodings to just one and takes into account given RTP capabilities
 * to reduce codecs, codecs' RTCP feedback and header extensions, and also enables
 * or disabled RTX.
 */
function getConsumerRtpParameters(consumableParams, caps) {
    const consumerParams = {
        codecs: [],
        headerExtensions: [],
        encodings: [],
        rtcp: consumableParams.rtcp
    };
    for (const capCodec of caps.codecs || []) {
        assertCodecCapability(capCodec);
    }
    const consumableCodecs = utils.clone(consumableParams.codecs || []);
    let rtxSupported = false;
    for (const codec of consumableCodecs) {
        const matchedCapCodec = caps.codecs
            .find((capCodec) => matchCodecs(capCodec, codec, { strict: true }));
        if (!matchedCapCodec)
            continue;
        codec.rtcpFeedback = matchedCapCodec.rtcpFeedback || [];
        consumerParams.codecs.push(codec);
        if (!rtxSupported && /.+\/rtx$/i.test(codec.mimeType))
            rtxSupported = true;
    }
    // Ensure there is at least one media codec.
    if (consumerParams.codecs.length === 0 ||
        /.+\/rtx$/i.test(consumerParams.codecs[0].mimeType)) {
        throw new errors_1.UnsupportedError('no compatible media codecs');
    }
    consumerParams.headerExtensions = consumableParams.headerExtensions
        .filter((ext) => ((caps.headerExtensions || [])
        .some((capExt) => capExt.preferredId === ext.id)));
    // Reduce codecs' RTCP feedback. Use Transport-CC if available, REMB otherwise.
    if (consumerParams.headerExtensions.some((ext) => (ext.uri === 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01'))) {
        for (const codec of consumerParams.codecs) {
            codec.rtcpFeedback = (codec.rtcpFeedback || [])
                .filter((fb) => fb.type !== 'goog-remb');
        }
    }
    else if (consumerParams.headerExtensions.some((ext) => (ext.uri === 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time'))) {
        for (const codec of consumerParams.codecs) {
            codec.rtcpFeedback = (codec.rtcpFeedback || [])
                .filter((fb) => fb.type !== 'transport-cc');
        }
    }
    else {
        for (const codec of consumerParams.codecs) {
            codec.rtcpFeedback = (codec.rtcpFeedback || [])
                .filter((fb) => (fb.type !== 'transport-cc' &&
                fb.type !== 'goog-remb'));
        }
    }
    const consumerEncoding = {
        ssrc: utils.generateRandomNumber()
    };
    if (rtxSupported)
        consumerEncoding.rtx = { ssrc: utils.generateRandomNumber() };
    // If any of the consumableParams.encodings has scalabilityMode, process it
    // (assume all encodings have the same value).
    const encodingWithScalabilityMode = consumableParams.encodings.find((encoding) => encoding.scalabilityMode);
    let scalabilityMode = encodingWithScalabilityMode
        ? encodingWithScalabilityMode.scalabilityMode
        : undefined;
    // If there is simulast, mangle spatial layers in scalabilityMode.
    if (consumableParams.encodings.length > 1) {
        const { temporalLayers } = scalabilityModes_1.parse(scalabilityMode);
        scalabilityMode = `S${consumableParams.encodings.length}T${temporalLayers}`;
    }
    if (scalabilityMode)
        consumerEncoding.scalabilityMode = scalabilityMode;
    // Set a single encoding for the Consumer.
    consumerParams.encodings.push(consumerEncoding);
    // Copy verbatim.
    consumerParams.rtcp = consumableParams.rtcp;
    return consumerParams;
}
exports.getConsumerRtpParameters = getConsumerRtpParameters;
/**
 * Generate RTP parameters for a pipe Consumer.
 *
 * It keeps all original consumable encodings, removes RTX support and also
 * other features such as NACK.
 */
function getPipeConsumerRtpParameters(consumableParams) {
    const consumerParams = {
        codecs: [],
        headerExtensions: [],
        encodings: [],
        rtcp: consumableParams.rtcp
    };
    const consumableCodecs = utils.clone(consumableParams.codecs || []);
    for (const codec of consumableCodecs) {
        if (/.+\/rtx$/i.test(codec.mimeType))
            continue;
        // Reduce RTCP feedbacks by removing NACK support and other features.
        codec.rtcpFeedback = codec.rtcpFeedback
            .filter((fb) => ((fb.type === 'nack' && fb.parameter === 'pli') ||
            (fb.type === 'ccm' && fb.parameter === 'fir')));
        consumerParams.codecs.push(codec);
    }
    // Reduce RTP extensions by disabling transport BWE related ones.
    consumerParams.headerExtensions = consumableParams.headerExtensions
        .filter((ext) => (ext.uri !== 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time' &&
        ext.uri !== 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01'));
    const consumableEncodings = utils.clone(consumableParams.encodings || []);
    for (const encoding of consumableEncodings) {
        delete encoding.rtx;
        consumerParams.encodings.push(encoding);
    }
    return consumerParams;
}
exports.getPipeConsumerRtpParameters = getPipeConsumerRtpParameters;
function assertCodecCapability(codec) {
    const valid = (typeof codec === 'object' && !Array.isArray(codec)) &&
        (typeof codec.mimeType === 'string' && codec.mimeType) &&
        (typeof codec.clockRate === 'number' && codec.clockRate);
    if (!valid)
        throw new TypeError('invalid RTCRtpCodecCapability');
    // Add kind if not present.
    if (!codec.kind)
        codec.kind = codec.mimeType.replace(/\/.*/, '').toLowerCase();
}
function assertCodecParameters(codec) {
    const valid = (typeof codec === 'object' && !Array.isArray(codec)) &&
        (typeof codec.mimeType === 'string' && codec.mimeType) &&
        (typeof codec.clockRate === 'number' && codec.clockRate);
    if (!valid)
        throw new TypeError('invalid RTCRtpCodecParameters');
}
function matchCodecs(aCodec, bCodec, { strict = false, modify = false } = {}) {
    const aMimeType = aCodec.mimeType.toLowerCase();
    const bMimeType = bCodec.mimeType.toLowerCase();
    if (aMimeType !== bMimeType)
        return false;
    if (aCodec.clockRate !== bCodec.clockRate)
        return false;
    if (/^audio\/.+$/i.test(aMimeType) &&
        ((aCodec.channels !== undefined && aCodec.channels !== 1) ||
            (bCodec.channels !== undefined && bCodec.channels !== 1)) &&
        aCodec.channels !== bCodec.channels) {
        return false;
    }
    // Per codec special checks.
    switch (aMimeType) {
        case 'video/h264':
            {
                const aPacketizationMode = (aCodec.parameters || {})['packetization-mode'] || 0;
                const bPacketizationMode = (bCodec.parameters || {})['packetization-mode'] || 0;
                if (aPacketizationMode !== bPacketizationMode)
                    return false;
                // If strict matching check profile-level-id.
                if (strict) {
                    if (!h264.isSameProfile(aCodec.parameters, bCodec.parameters))
                        return false;
                    let selectedProfileLevelId;
                    try {
                        selectedProfileLevelId =
                            h264.generateProfileLevelIdForAnswer(aCodec.parameters, bCodec.parameters);
                    }
                    catch (error) {
                        return false;
                    }
                    if (modify) {
                        aCodec.parameters = aCodec.parameters || {};
                        if (selectedProfileLevelId)
                            aCodec.parameters['profile-level-id'] = selectedProfileLevelId;
                        else
                            delete aCodec.parameters['profile-level-id'];
                    }
                }
                break;
            }
        case 'video/vp9':
            {
                // If strict matching check profile-id.
                if (strict) {
                    const aProfileId = (aCodec.parameters || {})['profile-id'] || 0;
                    const bProfileId = (bCodec.parameters || {})['profile-id'] || 0;
                    if (aProfileId !== bProfileId)
                        return false;
                }
                break;
            }
    }
    return true;
}
