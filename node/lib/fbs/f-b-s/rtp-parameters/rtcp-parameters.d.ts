import * as flatbuffers from 'flatbuffers';
export declare class RtcpParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RtcpParameters;
    static getRootAsRtcpParameters(bb: flatbuffers.ByteBuffer, obj?: RtcpParameters): RtcpParameters;
    static getSizePrefixedRootAsRtcpParameters(bb: flatbuffers.ByteBuffer, obj?: RtcpParameters): RtcpParameters;
    cname(): string | null;
    cname(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    reducedSize(): boolean;
    mux(): boolean;
    static startRtcpParameters(builder: flatbuffers.Builder): void;
    static addCname(builder: flatbuffers.Builder, cnameOffset: flatbuffers.Offset): void;
    static addReducedSize(builder: flatbuffers.Builder, reducedSize: boolean): void;
    static addMux(builder: flatbuffers.Builder, mux: boolean): void;
    static endRtcpParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createRtcpParameters(builder: flatbuffers.Builder, cnameOffset: flatbuffers.Offset, reducedSize: boolean, mux: boolean): flatbuffers.Offset;
}
//# sourceMappingURL=rtcp-parameters.d.ts.map