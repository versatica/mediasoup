import * as flatbuffers from 'flatbuffers';
export declare class EncodingMapping {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): EncodingMapping;
    static getRootAsEncodingMapping(bb: flatbuffers.ByteBuffer, obj?: EncodingMapping): EncodingMapping;
    static getSizePrefixedRootAsEncodingMapping(bb: flatbuffers.ByteBuffer, obj?: EncodingMapping): EncodingMapping;
    rid(): string | null;
    rid(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    ssrc(): number | null;
    scalabilityMode(): string | null;
    scalabilityMode(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    mappedSsrc(): number;
    static startEncodingMapping(builder: flatbuffers.Builder): void;
    static addRid(builder: flatbuffers.Builder, ridOffset: flatbuffers.Offset): void;
    static addSsrc(builder: flatbuffers.Builder, ssrc: number): void;
    static addScalabilityMode(builder: flatbuffers.Builder, scalabilityModeOffset: flatbuffers.Offset): void;
    static addMappedSsrc(builder: flatbuffers.Builder, mappedSsrc: number): void;
    static endEncodingMapping(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createEncodingMapping(builder: flatbuffers.Builder, ridOffset: flatbuffers.Offset, ssrc: number | null, scalabilityModeOffset: flatbuffers.Offset, mappedSsrc: number): flatbuffers.Offset;
    unpack(): EncodingMappingT;
    unpackTo(_o: EncodingMappingT): void;
}
export declare class EncodingMappingT {
    rid: string | Uint8Array | null;
    ssrc: number | null;
    scalabilityMode: string | Uint8Array | null;
    mappedSsrc: number;
    constructor(rid?: string | Uint8Array | null, ssrc?: number | null, scalabilityMode?: string | Uint8Array | null, mappedSsrc?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=encoding-mapping.d.ts.map