import * as flatbuffers from 'flatbuffers';
import { RtcpFeedback } from '../../f-b-s/transport/rtcp-feedback';
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
    parameters(): string | null;
    parameters(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    rtcpFeedback(index: number, obj?: RtcpFeedback): RtcpFeedback | null;
    rtcpFeedbackLength(): number;
    static startRtpCodecParameters(builder: flatbuffers.Builder): void;
    static addMimeType(builder: flatbuffers.Builder, mimeTypeOffset: flatbuffers.Offset): void;
    static addPayloadType(builder: flatbuffers.Builder, payloadType: number): void;
    static addClockRate(builder: flatbuffers.Builder, clockRate: number): void;
    static addChannels(builder: flatbuffers.Builder, channels: number): void;
    static addParameters(builder: flatbuffers.Builder, parametersOffset: flatbuffers.Offset): void;
    static addRtcpFeedback(builder: flatbuffers.Builder, rtcpFeedbackOffset: flatbuffers.Offset): void;
    static createRtcpFeedbackVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRtcpFeedbackVector(builder: flatbuffers.Builder, numElems: number): void;
    static endRtpCodecParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createRtpCodecParameters(builder: flatbuffers.Builder, mimeTypeOffset: flatbuffers.Offset, payloadType: number, clockRate: number, channels: number, parametersOffset: flatbuffers.Offset, rtcpFeedbackOffset: flatbuffers.Offset): flatbuffers.Offset;
}
//# sourceMappingURL=rtp-codec-parameters.d.ts.map