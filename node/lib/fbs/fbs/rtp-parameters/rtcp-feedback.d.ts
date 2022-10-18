import * as flatbuffers from 'flatbuffers';
export declare class RtcpFeedback {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RtcpFeedback;
    static getRootAsRtcpFeedback(bb: flatbuffers.ByteBuffer, obj?: RtcpFeedback): RtcpFeedback;
    static getSizePrefixedRootAsRtcpFeedback(bb: flatbuffers.ByteBuffer, obj?: RtcpFeedback): RtcpFeedback;
    type(): string | null;
    type(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    parameter(): string | null;
    parameter(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startRtcpFeedback(builder: flatbuffers.Builder): void;
    static addType(builder: flatbuffers.Builder, typeOffset: flatbuffers.Offset): void;
    static addParameter(builder: flatbuffers.Builder, parameterOffset: flatbuffers.Offset): void;
    static endRtcpFeedback(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createRtcpFeedback(builder: flatbuffers.Builder, typeOffset: flatbuffers.Offset, parameterOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): RtcpFeedbackT;
    unpackTo(_o: RtcpFeedbackT): void;
}
export declare class RtcpFeedbackT {
    type: string | Uint8Array | null;
    parameter: string | Uint8Array | null;
    constructor(type?: string | Uint8Array | null, parameter?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=rtcp-feedback.d.ts.map