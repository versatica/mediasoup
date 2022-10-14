import * as flatbuffers from 'flatbuffers';
export declare class Str {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Str;
    static getRootAsStr(bb: flatbuffers.ByteBuffer, obj?: Str): Str;
    static getSizePrefixedRootAsStr(bb: flatbuffers.ByteBuffer, obj?: Str): Str;
    value(): string | null;
    value(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startStr(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endStr(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createStr(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
}
//# sourceMappingURL=str.d.ts.map