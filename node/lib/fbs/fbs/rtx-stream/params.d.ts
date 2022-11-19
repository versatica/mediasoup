import * as flatbuffers from 'flatbuffers';
export declare class Params {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Params;
    static getRootAsParams(bb: flatbuffers.ByteBuffer, obj?: Params): Params;
    static getSizePrefixedRootAsParams(bb: flatbuffers.ByteBuffer, obj?: Params): Params;
    ssrc(): number;
    payloadType(): number;
    mimeType(): string | null;
    mimeType(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    clockRate(): number;
    rrid(): string | null;
    rrid(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    cname(): string | null;
    cname(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startParams(builder: flatbuffers.Builder): void;
    static addSsrc(builder: flatbuffers.Builder, ssrc: number): void;
    static addPayloadType(builder: flatbuffers.Builder, payloadType: number): void;
    static addMimeType(builder: flatbuffers.Builder, mimeTypeOffset: flatbuffers.Offset): void;
    static addClockRate(builder: flatbuffers.Builder, clockRate: number): void;
    static addRrid(builder: flatbuffers.Builder, rridOffset: flatbuffers.Offset): void;
    static addCname(builder: flatbuffers.Builder, cnameOffset: flatbuffers.Offset): void;
    static endParams(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createParams(builder: flatbuffers.Builder, ssrc: number, payloadType: number, mimeTypeOffset: flatbuffers.Offset, clockRate: number, rridOffset: flatbuffers.Offset, cnameOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): ParamsT;
    unpackTo(_o: ParamsT): void;
}
export declare class ParamsT {
    ssrc: number;
    payloadType: number;
    mimeType: string | Uint8Array | null;
    clockRate: number;
    rrid: string | Uint8Array | null;
    cname: string | Uint8Array | null;
    constructor(ssrc?: number, payloadType?: number, mimeType?: string | Uint8Array | null, clockRate?: number, rrid?: string | Uint8Array | null, cname?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=params.d.ts.map