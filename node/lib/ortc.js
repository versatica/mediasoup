"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.getPipeConsumerRtpParameters = exports.getConsumerRtpParameters = exports.canConsume = exports.getConsumableRtpParameters = exports.getProducerRtpParametersMapping = exports.generateRouterRtpCapabilities = exports.validateSctpStreamParameters = exports.validateSctpParameters = exports.validateNumSctpStreams = exports.validateSctpCapabilities = exports.validateRtcpParameters = exports.validateRtpEncodingParameters = exports.validateRtpHeaderExtensionParameters = exports.validateRtpCodecParameters = exports.validateRtpParameters = exports.validateRtpHeaderExtension = exports.validateRtcpFeedback = exports.validateRtpCodecCapability = exports.validateRtpCapabilities = void 0;
const h264 = require("h264-profile-level-id");
const utils = require("./utils");
const errors_1 = require("./errors");
const supportedRtpCapabilities_1 = require("./supportedRtpCapabilities");
const scalabilityModes_1 = require("./scalabilityModes");
const DynamicPayloadTypes = [
    100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110,
    111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
    122, 123, 124, 125, 126, 127, 96, 97, 98, 99
];
/**
 * Validates RtpCapabilities. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtpCapabilities(caps) {
    if (typeof caps !== 'object')
        throw new TypeError('caps is not an object');
    // codecs is optional. If unset, fill with an empty array.
    if (caps.codecs && !Array.isArray(caps.codecs))
        throw new TypeError('caps.codecs is not an array');
    else if (!caps.codecs)
        caps.codecs = [];
    for (const codec of caps.codecs) {
        validateRtpCodecCapability(codec);
    }
    // headerExtensions is optional. If unset, fill with an empty array.
    if (caps.headerExtensions && !Array.isArray(caps.headerExtensions))
        throw new TypeError('caps.headerExtensions is not an array');
    else if (!caps.headerExtensions)
        caps.headerExtensions = [];
    for (const ext of caps.headerExtensions) {
        validateRtpHeaderExtension(ext);
    }
}
exports.validateRtpCapabilities = validateRtpCapabilities;
/**
 * Validates RtpCodecCapability. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtpCodecCapability(codec) {
    const MimeTypeRegex = new RegExp('^(audio|video)/(.+)', 'i');
    if (typeof codec !== 'object')
        throw new TypeError('codec is not an object');
    // mimeType is mandatory.
    if (!codec.mimeType || typeof codec.mimeType !== 'string')
        throw new TypeError('missing codec.mimeType');
    const mimeTypeMatch = MimeTypeRegex.exec(codec.mimeType);
    if (!mimeTypeMatch)
        throw new TypeError('invalid codec.mimeType');
    // Just override kind with media component of mimeType.
    codec.kind = mimeTypeMatch[1].toLowerCase();
    // preferredPayloadType is optional.
    if (codec.preferredPayloadType && typeof codec.preferredPayloadType !== 'number')
        throw new TypeError('invalid codec.preferredPayloadType');
    // clockRate is mandatory.
    if (typeof codec.clockRate !== 'number')
        throw new TypeError('missing codec.clockRate');
    // channels is optional. If unset, set it to 1 (just if audio).
    if (codec.kind === 'audio') {
        if (typeof codec.channels !== 'number')
            codec.channels = 1;
    }
    else {
        delete codec.channels;
    }
    // parameters is optional. If unset, set it to an empty object.
    if (!codec.parameters || typeof codec.parameters !== 'object')
        codec.parameters = {};
    for (const key of Object.keys(codec.parameters)) {
        let value = codec.parameters[key];
        if (value === undefined) {
            codec.parameters[key] = '';
            value = '';
        }
        if (typeof value !== 'string' && typeof value !== 'number') {
            throw new TypeError(`invalid codec parameter [key:${key}s, value:${value}]`);
        }
        // Specific parameters validation.
        if (key === 'apt') {
            if (typeof value !== 'number')
                throw new TypeError('invalid codec apt parameter');
        }
    }
    // rtcpFeedback is optional. If unset, set it to an empty array.
    if (!codec.rtcpFeedback || !Array.isArray(codec.rtcpFeedback))
        codec.rtcpFeedback = [];
    for (const fb of codec.rtcpFeedback) {
        validateRtcpFeedback(fb);
    }
}
exports.validateRtpCodecCapability = validateRtpCodecCapability;
/**
 * Validates RtcpFeedback. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtcpFeedback(fb) {
    if (typeof fb !== 'object')
        throw new TypeError('fb is not an object');
    // type is mandatory.
    if (!fb.type || typeof fb.type !== 'string')
        throw new TypeError('missing fb.type');
    // parameter is optional. If unset set it to an empty string.
    if (!fb.parameter || typeof fb.parameter !== 'string')
        fb.parameter = '';
}
exports.validateRtcpFeedback = validateRtcpFeedback;
/**
 * Validates RtpHeaderExtension. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtpHeaderExtension(ext) {
    if (typeof ext !== 'object')
        throw new TypeError('ext is not an object');
    if (ext.kind !== 'audio' && ext.kind !== 'video')
        throw new TypeError('invalid ext.kind');
    // uri is mandatory.
    if (!ext.uri || typeof ext.uri !== 'string')
        throw new TypeError('missing ext.uri');
    // preferredId is mandatory.
    if (typeof ext.preferredId !== 'number')
        throw new TypeError('missing ext.preferredId');
    // preferredEncrypt is optional. If unset set it to false.
    if (ext.preferredEncrypt && typeof ext.preferredEncrypt !== 'boolean')
        throw new TypeError('invalid ext.preferredEncrypt');
    else if (!ext.preferredEncrypt)
        ext.preferredEncrypt = false;
    // direction is optional. If unset set it to sendrecv.
    if (ext.direction && typeof ext.direction !== 'string')
        throw new TypeError('invalid ext.direction');
    else if (!ext.direction)
        ext.direction = 'sendrecv';
}
exports.validateRtpHeaderExtension = validateRtpHeaderExtension;
/**
 * Validates RtpParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtpParameters(params) {
    if (typeof params !== 'object')
        throw new TypeError('params is not an object');
    // mid is optional.
    if (params.mid && typeof params.mid !== 'string')
        throw new TypeError('params.mid is not a string');
    // codecs is mandatory.
    if (!Array.isArray(params.codecs))
        throw new TypeError('missing params.codecs');
    for (const codec of params.codecs) {
        validateRtpCodecParameters(codec);
    }
    // headerExtensions is optional. If unset, fill with an empty array.
    if (params.headerExtensions && !Array.isArray(params.headerExtensions))
        throw new TypeError('params.headerExtensions is not an array');
    else if (!params.headerExtensions)
        params.headerExtensions = [];
    for (const ext of params.headerExtensions) {
        validateRtpHeaderExtensionParameters(ext);
    }
    // encodings is optional. If unset, fill with an empty array.
    if (params.encodings && !Array.isArray(params.encodings))
        throw new TypeError('params.encodings is not an array');
    else if (!params.encodings)
        params.encodings = [];
    for (const encoding of params.encodings) {
        validateRtpEncodingParameters(encoding);
    }
    // rtcp is optional. If unset, fill with an empty object.
    if (params.rtcp && typeof params.rtcp !== 'object')
        throw new TypeError('params.rtcp is not an object');
    else if (!params.rtcp)
        params.rtcp = {};
    validateRtcpParameters(params.rtcp);
}
exports.validateRtpParameters = validateRtpParameters;
/**
 * Validates RtpCodecParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtpCodecParameters(codec) {
    const MimeTypeRegex = new RegExp('^(audio|video)/(.+)', 'i');
    if (typeof codec !== 'object')
        throw new TypeError('codec is not an object');
    // mimeType is mandatory.
    if (!codec.mimeType || typeof codec.mimeType !== 'string')
        throw new TypeError('missing codec.mimeType');
    const mimeTypeMatch = MimeTypeRegex.exec(codec.mimeType);
    if (!mimeTypeMatch)
        throw new TypeError('invalid codec.mimeType');
    // payloadType is mandatory.
    if (typeof codec.payloadType !== 'number')
        throw new TypeError('missing codec.payloadType');
    // clockRate is mandatory.
    if (typeof codec.clockRate !== 'number')
        throw new TypeError('missing codec.clockRate');
    const kind = mimeTypeMatch[1].toLowerCase();
    // channels is optional. If unset, set it to 1 (just if audio).
    if (kind === 'audio') {
        if (typeof codec.channels !== 'number')
            codec.channels = 1;
    }
    else {
        delete codec.channels;
    }
    // parameters is optional. If unset, set it to an empty object.
    if (!codec.parameters || typeof codec.parameters !== 'object')
        codec.parameters = {};
    for (const key of Object.keys(codec.parameters)) {
        let value = codec.parameters[key];
        if (value === undefined) {
            codec.parameters[key] = '';
            value = '';
        }
        if (typeof value !== 'string' && typeof value !== 'number') {
            throw new TypeError(`invalid codec parameter [key:${key}s, value:${value}]`);
        }
        // Specific parameters validation.
        if (key === 'apt') {
            if (typeof value !== 'number')
                throw new TypeError('invalid codec apt parameter');
        }
    }
    // rtcpFeedback is optional. If unset, set it to an empty array.
    if (!codec.rtcpFeedback || !Array.isArray(codec.rtcpFeedback))
        codec.rtcpFeedback = [];
    for (const fb of codec.rtcpFeedback) {
        validateRtcpFeedback(fb);
    }
}
exports.validateRtpCodecParameters = validateRtpCodecParameters;
/**
 * Validates RtpHeaderExtensionParameteters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtpHeaderExtensionParameters(ext) {
    if (typeof ext !== 'object')
        throw new TypeError('ext is not an object');
    // uri is mandatory.
    if (!ext.uri || typeof ext.uri !== 'string')
        throw new TypeError('missing ext.uri');
    // id is mandatory.
    if (typeof ext.id !== 'number')
        throw new TypeError('missing ext.id');
    // encrypt is optional. If unset set it to false.
    if (ext.encrypt && typeof ext.encrypt !== 'boolean')
        throw new TypeError('invalid ext.encrypt');
    else if (!ext.encrypt)
        ext.encrypt = false;
    // parameters is optional. If unset, set it to an empty object.
    if (!ext.parameters || typeof ext.parameters !== 'object')
        ext.parameters = {};
    for (const key of Object.keys(ext.parameters)) {
        let value = ext.parameters[key];
        if (value === undefined) {
            ext.parameters[key] = '';
            value = '';
        }
        if (typeof value !== 'string' && typeof value !== 'number')
            throw new TypeError('invalid header extension parameter');
    }
}
exports.validateRtpHeaderExtensionParameters = validateRtpHeaderExtensionParameters;
/**
 * Validates RtpEncodingParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtpEncodingParameters(encoding) {
    if (typeof encoding !== 'object')
        throw new TypeError('encoding is not an object');
    // ssrc is optional.
    if (encoding.ssrc && typeof encoding.ssrc !== 'number')
        throw new TypeError('invalid encoding.ssrc');
    // rid is optional.
    if (encoding.rid && typeof encoding.rid !== 'string')
        throw new TypeError('invalid encoding.rid');
    // rtx is optional.
    if (encoding.rtx && typeof encoding.rtx !== 'object') {
        throw new TypeError('invalid encoding.rtx');
    }
    else if (encoding.rtx) {
        // RTX ssrc is mandatory if rtx is present.
        if (typeof encoding.rtx.ssrc !== 'number')
            throw new TypeError('missing encoding.rtx.ssrc');
    }
    // dtx is optional. If unset set it to false.
    if (!encoding.dtx || typeof encoding.dtx !== 'boolean')
        encoding.dtx = false;
    // scalabilityMode is optional.
    if (encoding.scalabilityMode && typeof encoding.scalabilityMode !== 'string')
        throw new TypeError('invalid encoding.scalabilityMode');
}
exports.validateRtpEncodingParameters = validateRtpEncodingParameters;
/**
 * Validates RtcpParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateRtcpParameters(rtcp) {
    if (typeof rtcp !== 'object')
        throw new TypeError('rtcp is not an object');
    // cname is optional.
    if (rtcp.cname && typeof rtcp.cname !== 'string')
        throw new TypeError('invalid rtcp.cname');
    // reducedSize is optional. If unset set it to true.
    if (!rtcp.reducedSize || typeof rtcp.reducedSize !== 'boolean')
        rtcp.reducedSize = true;
}
exports.validateRtcpParameters = validateRtcpParameters;
/**
 * Validates SctpCapabilities. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateSctpCapabilities(caps) {
    if (typeof caps !== 'object')
        throw new TypeError('caps is not an object');
    // numStreams is mandatory.
    if (!caps.numStreams || typeof caps.numStreams !== 'object')
        throw new TypeError('missing caps.numStreams');
    validateNumSctpStreams(caps.numStreams);
}
exports.validateSctpCapabilities = validateSctpCapabilities;
/**
 * Validates NumSctpStreams. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateNumSctpStreams(numStreams) {
    if (typeof numStreams !== 'object')
        throw new TypeError('numStreams is not an object');
    // OS is mandatory.
    if (typeof numStreams.OS !== 'number')
        throw new TypeError('missing numStreams.OS');
    // MIS is mandatory.
    if (typeof numStreams.MIS !== 'number')
        throw new TypeError('missing numStreams.MIS');
}
exports.validateNumSctpStreams = validateNumSctpStreams;
/**
 * Validates SctpParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateSctpParameters(params) {
    if (typeof params !== 'object')
        throw new TypeError('params is not an object');
    // port is mandatory.
    if (typeof params.port !== 'number')
        throw new TypeError('missing params.port');
    // OS is mandatory.
    if (typeof params.OS !== 'number')
        throw new TypeError('missing params.OS');
    // MIS is mandatory.
    if (typeof params.MIS !== 'number')
        throw new TypeError('missing params.MIS');
    // maxMessageSize is mandatory.
    if (typeof params.maxMessageSize !== 'number')
        throw new TypeError('missing params.maxMessageSize');
}
exports.validateSctpParameters = validateSctpParameters;
/**
 * Validates SctpStreamParameters. It may modify given data by adding missing
 * fields with default values.
 * It throws if invalid.
 */
function validateSctpStreamParameters(params) {
    if (typeof params !== 'object')
        throw new TypeError('params is not an object');
    // streamId is mandatory.
    if (typeof params.streamId !== 'number')
        throw new TypeError('missing params.streamId');
    // ordered is optional.
    let orderedGiven = false;
    if (typeof params.ordered === 'boolean')
        orderedGiven = true;
    else
        params.ordered = true;
    // maxPacketLifeTime is optional.
    if (params.maxPacketLifeTime && typeof params.maxPacketLifeTime !== 'number')
        throw new TypeError('invalid params.maxPacketLifeTime');
    // maxRetransmits is optional.
    if (params.maxRetransmits && typeof params.maxRetransmits !== 'number')
        throw new TypeError('invalid params.maxRetransmits');
    if (params.maxPacketLifeTime && params.maxRetransmits)
        throw new TypeError('cannot provide both maxPacketLifeTime and maxRetransmits');
    if (orderedGiven &&
        params.ordered &&
        (params.maxPacketLifeTime || params.maxRetransmits)) {
        throw new TypeError('cannot be ordered with maxPacketLifeTime or maxRetransmits');
    }
    else if (!orderedGiven && (params.maxPacketLifeTime || params.maxRetransmits)) {
        params.ordered = false;
    }
}
exports.validateSctpStreamParameters = validateSctpStreamParameters;
/**
 * Generate RTP capabilities for the Router based on the given media codecs and
 * mediasoup supported RTP capabilities.
 */
function generateRouterRtpCapabilities(mediaCodecs = []) {
    // Normalize supported RTP capabilities.
    validateRtpCapabilities(supportedRtpCapabilities_1.supportedRtpCapabilities);
    if (!Array.isArray(mediaCodecs))
        throw new TypeError('mediaCodecs must be an Array');
    const clonedSupportedRtpCapabilities = utils.clone(supportedRtpCapabilities_1.supportedRtpCapabilities);
    const dynamicPayloadTypes = utils.clone(DynamicPayloadTypes);
    const caps = {
        codecs: [],
        headerExtensions: clonedSupportedRtpCapabilities.headerExtensions
    };
    for (const mediaCodec of mediaCodecs) {
        // This may throw.
        validateRtpCodecCapability(mediaCodec);
        const matchedSupportedCodec = clonedSupportedRtpCapabilities
            .codecs
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
            throw new TypeError('duplicated codec.preferredPayloadType');
        // Merge the media codec parameters.
        codec.parameters = { ...codec.parameters, ...mediaCodec.parameters };
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
                parameters: {
                    apt: codec.preferredPayloadType
                },
                rtcpFeedback: []
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
    for (const codec of params.codecs) {
        if (isRtxCodec(codec))
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
    for (const codec of params.codecs) {
        if (!isRtxCodec(codec))
            continue;
        // Search for the associated media codec.
        const associatedMediaCodec = params.codecs
            .find((mediaCodec) => mediaCodec.payloadType === codec.parameters.apt);
        if (!associatedMediaCodec) {
            throw new TypeError(`missing media codec found for RTX PT ${codec.payloadType}`);
        }
        const capMediaCodec = codecToCapCodec.get(associatedMediaCodec);
        // Ensure that the capabilities media codec has a RTX codec.
        const associatedCapRtxCodec = caps.codecs
            .find((capCodec) => (isRtxCodec(capCodec) &&
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
    for (const encoding of params.encodings) {
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
    for (const codec of params.codecs) {
        if (isRtxCodec(codec))
            continue;
        const consumableCodecPt = rtpMapping.codecs
            .find((entry) => entry.payloadType === codec.payloadType)
            .mappedPayloadType;
        const matchedCapCodec = caps.codecs
            .find((capCodec) => capCodec.preferredPayloadType === consumableCodecPt);
        const consumableCodec = {
            mimeType: matchedCapCodec.mimeType,
            payloadType: matchedCapCodec.preferredPayloadType,
            clockRate: matchedCapCodec.clockRate,
            channels: matchedCapCodec.channels,
            parameters: codec.parameters,
            rtcpFeedback: matchedCapCodec.rtcpFeedback
        };
        consumableParams.codecs.push(consumableCodec);
        const consumableCapRtxCodec = caps.codecs
            .find((capRtxCodec) => (isRtxCodec(capRtxCodec) &&
            capRtxCodec.parameters.apt === consumableCodec.payloadType));
        if (consumableCapRtxCodec) {
            const consumableRtxCodec = {
                mimeType: consumableCapRtxCodec.mimeType,
                payloadType: consumableCapRtxCodec.preferredPayloadType,
                clockRate: consumableCapRtxCodec.clockRate,
                parameters: consumableCapRtxCodec.parameters,
                rtcpFeedback: consumableCapRtxCodec.rtcpFeedback
            };
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
            id: capExt.preferredId,
            encrypt: capExt.preferredEncrypt,
            parameters: {}
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
    // This may throw.
    validateRtpCapabilities(caps);
    const matchingCodecs = [];
    for (const codec of consumableParams.codecs) {
        const matchedCapCodec = caps.codecs
            .find((capCodec) => matchCodecs(capCodec, codec, { strict: true }));
        if (!matchedCapCodec)
            continue;
        matchingCodecs.push(codec);
    }
    // Ensure there is at least one media codec.
    if (matchingCodecs.length === 0 || isRtxCodec(matchingCodecs[0]))
        return false;
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
function getConsumerRtpParameters(consumableParams, caps, pipe) {
    const consumerParams = {
        codecs: [],
        headerExtensions: [],
        encodings: [],
        rtcp: consumableParams.rtcp
    };
    for (const capCodec of caps.codecs) {
        validateRtpCodecCapability(capCodec);
    }
    const consumableCodecs = utils.clone(consumableParams.codecs);
    let rtxSupported = false;
    for (const codec of consumableCodecs) {
        const matchedCapCodec = caps.codecs
            .find((capCodec) => matchCodecs(capCodec, codec, { strict: true }));
        if (!matchedCapCodec)
            continue;
        codec.rtcpFeedback = matchedCapCodec.rtcpFeedback;
        consumerParams.codecs.push(codec);
    }
    // Must sanitize the list of matched codecs by removing useless RTX codecs.
    for (let idx = consumerParams.codecs.length - 1; idx >= 0; --idx) {
        const codec = consumerParams.codecs[idx];
        if (isRtxCodec(codec)) {
            // Search for the associated media codec.
            const associatedMediaCodec = consumerParams.codecs
                .find((mediaCodec) => mediaCodec.payloadType === codec.parameters.apt);
            if (associatedMediaCodec)
                rtxSupported = true;
            else
                consumerParams.codecs.splice(idx, 1);
        }
    }
    // Ensure there is at least one media codec.
    if (consumerParams.codecs.length === 0 || isRtxCodec(consumerParams.codecs[0])) {
        throw new errors_1.UnsupportedError('no compatible media codecs');
    }
    consumerParams.headerExtensions = consumableParams.headerExtensions
        .filter((ext) => (caps.headerExtensions
        .some((capExt) => (capExt.preferredId === ext.id &&
        capExt.uri === ext.uri))));
    // Reduce codecs' RTCP feedback. Use Transport-CC if available, REMB otherwise.
    if (consumerParams.headerExtensions.some((ext) => (ext.uri === 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01'))) {
        for (const codec of consumerParams.codecs) {
            codec.rtcpFeedback = codec.rtcpFeedback
                .filter((fb) => fb.type !== 'goog-remb');
        }
    }
    else if (consumerParams.headerExtensions.some((ext) => (ext.uri === 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time'))) {
        for (const codec of consumerParams.codecs) {
            codec.rtcpFeedback = codec.rtcpFeedback
                .filter((fb) => fb.type !== 'transport-cc');
        }
    }
    else {
        for (const codec of consumerParams.codecs) {
            codec.rtcpFeedback = codec.rtcpFeedback
                .filter((fb) => (fb.type !== 'transport-cc' &&
                fb.type !== 'goog-remb'));
        }
    }
    if (!pipe) {
        const consumerEncoding = {
            ssrc: utils.generateRandomNumber()
        };
        if (rtxSupported)
            consumerEncoding.rtx = { ssrc: consumerEncoding.ssrc + 1 };
        // If any of the consumableParams.encodings has scalabilityMode, process it
        // (assume all encodings have the same value).
        const encodingWithScalabilityMode = consumableParams.encodings.find((encoding) => encoding.scalabilityMode);
        let scalabilityMode = encodingWithScalabilityMode
            ? encodingWithScalabilityMode.scalabilityMode
            : undefined;
        // If there is simulast, mangle spatial layers in scalabilityMode.
        if (consumableParams.encodings.length > 1) {
            const { temporalLayers } = (0, scalabilityModes_1.parse)(scalabilityMode);
            scalabilityMode = `S${consumableParams.encodings.length}T${temporalLayers}`;
        }
        if (scalabilityMode)
            consumerEncoding.scalabilityMode = scalabilityMode;
        // Use the maximum maxBitrate in any encoding and honor it in the Consumer's
        // encoding.
        const maxEncodingMaxBitrate = consumableParams.encodings.reduce((maxBitrate, encoding) => (encoding.maxBitrate && encoding.maxBitrate > maxBitrate
            ? encoding.maxBitrate
            : maxBitrate), 0);
        if (maxEncodingMaxBitrate) {
            consumerEncoding.maxBitrate = maxEncodingMaxBitrate;
        }
        // Set a single encoding for the Consumer.
        consumerParams.encodings.push(consumerEncoding);
    }
    else {
        const consumableEncodings = utils.clone(consumableParams.encodings);
        const baseSsrc = utils.generateRandomNumber();
        const baseRtxSsrc = utils.generateRandomNumber();
        for (let i = 0; i < consumableEncodings.length; ++i) {
            const encoding = consumableEncodings[i];
            encoding.ssrc = baseSsrc + i;
            if (rtxSupported)
                encoding.rtx = { ssrc: baseRtxSsrc + i };
            else
                delete encoding.rtx;
            consumerParams.encodings.push(encoding);
        }
    }
    return consumerParams;
}
exports.getConsumerRtpParameters = getConsumerRtpParameters;
/**
 * Generate RTP parameters for a pipe Consumer.
 *
 * It keeps all original consumable encodings and removes support for BWE. If
 * enableRtx is false, it also removes RTX and NACK support.
 */
function getPipeConsumerRtpParameters(consumableParams, enableRtx = false) {
    const consumerParams = {
        codecs: [],
        headerExtensions: [],
        encodings: [],
        rtcp: consumableParams.rtcp
    };
    const consumableCodecs = utils.clone(consumableParams.codecs);
    for (const codec of consumableCodecs) {
        if (!enableRtx && isRtxCodec(codec))
            continue;
        codec.rtcpFeedback = codec.rtcpFeedback
            .filter((fb) => ((fb.type === 'nack' && fb.parameter === 'pli') ||
            (fb.type === 'ccm' && fb.parameter === 'fir') ||
            (enableRtx && fb.type === 'nack' && !fb.parameter)));
        consumerParams.codecs.push(codec);
    }
    // Reduce RTP extensions by disabling transport MID and BWE related ones.
    consumerParams.headerExtensions = consumableParams.headerExtensions
        .filter((ext) => (ext.uri !== 'urn:ietf:params:rtp-hdrext:sdes:mid' &&
        ext.uri !== 'http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time' &&
        ext.uri !== 'http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01'));
    const consumableEncodings = utils.clone(consumableParams.encodings);
    const baseSsrc = utils.generateRandomNumber();
    const baseRtxSsrc = utils.generateRandomNumber();
    for (let i = 0; i < consumableEncodings.length; ++i) {
        const encoding = consumableEncodings[i];
        encoding.ssrc = baseSsrc + i;
        if (enableRtx)
            encoding.rtx = { ssrc: baseRtxSsrc + i };
        else
            delete encoding.rtx;
        consumerParams.encodings.push(encoding);
    }
    return consumerParams;
}
exports.getPipeConsumerRtpParameters = getPipeConsumerRtpParameters;
function isRtxCodec(codec) {
    return /.+\/rtx$/i.test(codec.mimeType);
}
function matchCodecs(aCodec, bCodec, { strict = false, modify = false } = {}) {
    const aMimeType = aCodec.mimeType.toLowerCase();
    const bMimeType = bCodec.mimeType.toLowerCase();
    if (aMimeType !== bMimeType)
        return false;
    if (aCodec.clockRate !== bCodec.clockRate)
        return false;
    if (aCodec.channels !== bCodec.channels)
        return false;
    // Per codec special checks.
    switch (aMimeType) {
        case 'audio/multiopus':
            {
                const aNumStreams = aCodec.parameters['num_streams'];
                const bNumStreams = bCodec.parameters['num_streams'];
                if (aNumStreams !== bNumStreams)
                    return false;
                const aCoupledStreams = aCodec.parameters['coupled_streams'];
                const bCoupledStreams = bCodec.parameters['coupled_streams'];
                if (aCoupledStreams !== bCoupledStreams)
                    return false;
                break;
            }
        case 'video/h264':
            {
                if (strict) {
                    const aPacketizationMode = aCodec.parameters['packetization-mode'] || 0;
                    const bPacketizationMode = bCodec.parameters['packetization-mode'] || 0;
                    if (aPacketizationMode !== bPacketizationMode)
                        return false;
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
                if (strict) {
                    const aProfileId = aCodec.parameters['profile-id'] || 0;
                    const bProfileId = bCodec.parameters['profile-id'] || 0;
                    if (aProfileId !== bProfileId)
                        return false;
                }
                break;
            }
    }
    return true;
}
