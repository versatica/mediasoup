import * as flatbuffers from 'flatbuffers';
import { Parameter, ParameterT } from '../../fbs/rtp-parameters/parameter';
import { RtcpFeedback, RtcpFeedbackT } from '../../fbs/rtp-parameters/rtcp-feedback';
export declare class RtpCodecParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RtpCodecParameters;
    static getRootAsRtpCodecParameters(bb: flatbuffers.ByteBuffer, obj?: RtpCodecParameters): RtpCodecParameters;
    static getSizePrefixedRootAsRtpCodecParameters(bb: flatbuffers.ByteBuffer, obj?: RtpCodecParameters): RtpCodecParameters;
    mimeType(): string | null;
    mimeType(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    payloadType(): number;
    clockRate(): number;
    channels(): number;
    parameters(index: number, obj?: Parameter): Parameter | null;
    parametersLength(): number;
    rtcpFeedback(index: number, obj?: RtcpFeedback): RtcpFeedback | null;
    rtcpFeedbackLength(): number;
    static startRtpCodecParameters(builder: flatbuffers.Builder): void;
    static addMimeType(builder: flatbuffers.Builder, mimeTypeOffset: flatbuffers.Offset): void;
    static addPayloadType(builder: flatbuffers.Builder, payloadType: number): void;
    static addClockRate(builder: flatbuffers.Builder, clockRate: number): void;
    static addChannels(builder: flatbuffers.Builder, channels: number): void;
    static addParameters(builder: flatbuffers.Builder, parametersOffset: flatbuffers.Offset): void;
    static createParametersVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startParametersVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRtcpFeedback(builder: flatbuffers.Builder, rtcpFeedbackOffset: flatbuffers.Offset): void;
    static createRtcpFeedbackVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRtcpFeedbackVector(builder: flatbuffers.Builder, numElems: number): void;
    static endRtpCodecParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createRtpCodecParameters(builder: flatbuffers.Builder, mimeTypeOffset: flatbuffers.Offset, payloadType: number, clockRate: number, channels: number, parametersOffset: flatbuffers.Offset, rtcpFeedbackOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): RtpCodecParametersT;
    unpackTo(_o: RtpCodecParametersT): void;
}
export declare class RtpCodecParametersT {
    mimeType: string | Uint8Array | null;
    payloadType: number;
    clockRate: number;
    channels: number;
    parameters: (ParameterT)[];
    rtcpFeedback: (RtcpFeedbackT)[];
    constructor(mimeType?: string | Uint8Array | null, payloadType?: number, clockRate?: number, channels?: number, parameters?: (ParameterT)[], rtcpFeedback?: (RtcpFeedbackT)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=rtp-codec-parameters.d.ts.map