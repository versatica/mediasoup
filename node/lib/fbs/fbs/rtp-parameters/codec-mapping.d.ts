import * as flatbuffers from 'flatbuffers';
export declare class CodecMapping {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CodecMapping;
    static getRootAsCodecMapping(bb: flatbuffers.ByteBuffer, obj?: CodecMapping): CodecMapping;
    static getSizePrefixedRootAsCodecMapping(bb: flatbuffers.ByteBuffer, obj?: CodecMapping): CodecMapping;
    payloadType(): number;
    mappedPayloadType(): number;
    static startCodecMapping(builder: flatbuffers.Builder): void;
    static addPayloadType(builder: flatbuffers.Builder, payloadType: number): void;
    static addMappedPayloadType(builder: flatbuffers.Builder, mappedPayloadType: number): void;
    static endCodecMapping(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCodecMapping(builder: flatbuffers.Builder, payloadType: number, mappedPayloadType: number): flatbuffers.Offset;
    unpack(): CodecMappingT;
    unpackTo(_o: CodecMappingT): void;
}
export declare class CodecMappingT {
    payloadType: number;
    mappedPayloadType: number;
    constructor(payloadType?: number, mappedPayloadType?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=codec-mapping.d.ts.map