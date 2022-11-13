"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.serializeRtpEncodingParameters = exports.serializeRtpParameters = void 0;
const rtpParameters_generated_1 = require("./fbs/rtpParameters_generated");
function serializeRtpParameters(builder, rtpParameters) {
    const codecs = [];
    const headerExtensions = [];
    for (const codec of rtpParameters.codecs) {
        const mimeTypeOffset = builder.createString(codec.mimeType);
        const codecParameters = [];
        for (const key of Object.keys(codec.parameters)) {
            const value = codec.parameters[key];
            const keyOffset = builder.createString(key);
            let parameterOffset;
            if (typeof value === 'boolean') {
                parameterOffset = rtpParameters_generated_1.Parameter.createParameter(builder, keyOffset, rtpParameters_generated_1.Value.Boolean, value === true ? 1 : 0);
            }
            else if (typeof value === 'number') {
                // Integer.
                if (value % 1 === 0) {
                    const valueOffset = rtpParameters_generated_1.Integer.createInteger(builder, value);
                    parameterOffset = rtpParameters_generated_1.Parameter.createParameter(builder, keyOffset, rtpParameters_generated_1.Value.Integer, valueOffset);
                }
                // Float.
                else {
                    const valueOffset = rtpParameters_generated_1.Double.createDouble(builder, value);
                    parameterOffset = rtpParameters_generated_1.Parameter.createParameter(builder, keyOffset, rtpParameters_generated_1.Value.Double, valueOffset);
                }
            }
            else if (typeof value === 'string') {
                const valueOffset = rtpParameters_generated_1.String.createString(builder, builder.createString(value));
                parameterOffset = rtpParameters_generated_1.Parameter.createParameter(builder, keyOffset, rtpParameters_generated_1.Value.String, valueOffset);
            }
            else if (Array.isArray(value)) {
                const valueOffset = rtpParameters_generated_1.IntegerArray.createValueVector(builder, value);
                parameterOffset = rtpParameters_generated_1.Parameter.createParameter(builder, keyOffset, rtpParameters_generated_1.Value.IntegerArray, valueOffset);
            }
            else {
                throw new Error(`invalid parameter type [key:'${key}', value:${value}]`);
            }
            codecParameters.push(parameterOffset);
        }
        const parametersOffset = rtpParameters_generated_1.RtpCodecParameters.createParametersVector(builder, codecParameters);
        const rtcpFeedback = [];
        for (const rtcp of codec.rtcpFeedback ?? []) {
            const typeOffset = builder.createString(rtcp.type);
            const rtcpParametersOffset = builder.createString(rtcp.parameter);
            rtcpFeedback.push(rtpParameters_generated_1.RtcpFeedback.createRtcpFeedback(builder, typeOffset, rtcpParametersOffset));
        }
        const rtcpFeedbackOffset = rtpParameters_generated_1.RtpCodecParameters.createRtcpFeedbackVector(builder, rtcpFeedback);
        codecs.push(rtpParameters_generated_1.RtpCodecParameters.createRtpCodecParameters(builder, mimeTypeOffset, codec.payloadType, codec.clockRate, Number(codec.channels), parametersOffset, rtcpFeedbackOffset));
    }
    const codecsOffset = rtpParameters_generated_1.RtpParameters.createCodecsVector(builder, codecs);
    // RtpHeaderExtensionParameters.
    for (const headerExtension of rtpParameters.headerExtensions ?? []) {
        const uriOffset = builder.createString(headerExtension.uri);
        const parametersOffset = builder.createString(headerExtension.parameters);
        headerExtensions.push(rtpParameters_generated_1.RtpHeaderExtensionParameters.createRtpHeaderExtensionParameters(builder, uriOffset, headerExtension.id, Boolean(headerExtension.encrypt), parametersOffset));
    }
    const headerExtensionsOffset = rtpParameters_generated_1.RtpParameters.createHeaderExtensionsVector(builder, headerExtensions);
    // RtpEncodingParameters.
    let encodingsOffset;
    if (rtpParameters.encodings)
        encodingsOffset = serializeRtpEncodingParameters(builder, rtpParameters.encodings);
    // RtcpParameters.
    let rtcpOffset;
    if (rtpParameters.rtcp) {
        const { cname, reducedSize, mux } = rtpParameters.rtcp;
        const cnameOffset = builder.createString(cname);
        rtcpOffset = rtpParameters_generated_1.RtcpParameters.createRtcpParameters(builder, cnameOffset, Boolean(reducedSize), Boolean(mux));
    }
    const midOffset = builder.createString(rtpParameters.mid);
    rtpParameters_generated_1.RtpParameters.startRtpParameters(builder);
    rtpParameters_generated_1.RtpParameters.addMid(builder, midOffset);
    rtpParameters_generated_1.RtpParameters.addCodecs(builder, codecsOffset);
    if (headerExtensions.length > 0)
        rtpParameters_generated_1.RtpParameters.addHeaderExtensions(builder, headerExtensionsOffset);
    if (encodingsOffset)
        rtpParameters_generated_1.RtpParameters.addEncodings(builder, encodingsOffset);
    if (rtcpOffset)
        rtpParameters_generated_1.RtpParameters.addRtcp(builder, rtcpOffset);
    return rtpParameters_generated_1.RtpParameters.endRtpParameters(builder);
}
exports.serializeRtpParameters = serializeRtpParameters;
function serializeRtpEncodingParameters(builder, rtpEncodingParameters) {
    const encodings = [];
    for (const encoding of rtpEncodingParameters ?? []) {
        // Prepare Rid.
        const ridOffset = builder.createString(encoding.rid);
        // Prepare Rtx.
        let rtxOffset;
        if (encoding.rtx)
            rtxOffset = rtpParameters_generated_1.Rtx.createRtx(builder, encoding.rtx.ssrc);
        // Prepare scalability mode.
        let scalabilityModeOffset;
        if (encoding.scalabilityMode)
            scalabilityModeOffset = builder.createString(encoding.scalabilityMode);
        // Start serialization.
        rtpParameters_generated_1.RtpEncodingParameters.startRtpEncodingParameters(builder);
        // Add SSRC.
        if (encoding.ssrc)
            rtpParameters_generated_1.RtpEncodingParameters.addSsrc(builder, encoding.ssrc);
        // Add Rid.
        rtpParameters_generated_1.RtpEncodingParameters.addRid(builder, ridOffset);
        // Add payload type.
        if (encoding.codecPayloadType)
            rtpParameters_generated_1.RtpEncodingParameters.addCodecPayloadType(builder, encoding.codecPayloadType);
        // Add RTX.
        if (rtxOffset)
            rtpParameters_generated_1.RtpEncodingParameters.addRtx(builder, rtxOffset);
        // Add DTX.
        if (encoding.dtx !== undefined)
            rtpParameters_generated_1.RtpEncodingParameters.addDtx(builder, encoding.dtx);
        // Add scalability ode.
        if (scalabilityModeOffset)
            rtpParameters_generated_1.RtpEncodingParameters.addScalabilityMode(builder, scalabilityModeOffset);
        // Add scale resolution down by.
        if (encoding.scaleResolutionDownBy !== undefined) {
            rtpParameters_generated_1.RtpEncodingParameters.addScaleResolutionDownBy(builder, encoding.scaleResolutionDownBy);
        }
        // Add max bitrate.
        if (encoding.maxBitrate !== undefined)
            rtpParameters_generated_1.RtpEncodingParameters.addMaxBitrate(builder, encoding.maxBitrate);
        // End serialization.
        encodings.push(rtpParameters_generated_1.RtpEncodingParameters.endRtpEncodingParameters(builder));
    }
    return rtpParameters_generated_1.RtpParameters.createEncodingsVector(builder, encodings);
}
exports.serializeRtpEncodingParameters = serializeRtpEncodingParameters;
