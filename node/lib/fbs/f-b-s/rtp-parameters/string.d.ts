import * as flatbuffers from 'flatbuffers';
export declare class String {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): String;
    static getRootAsString(bb: flatbuffers.ByteBuffer, obj?: String): String;
    static getSizePrefixedRootAsString(bb: flatbuffers.ByteBuffer, obj?: String): String;
    value(): string | null;
    value(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startString(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endString(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createString(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
}
//# sourceMappingURL=string.d.ts.map