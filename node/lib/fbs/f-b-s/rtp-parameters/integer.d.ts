import * as flatbuffers from 'flatbuffers';
export declare class Integer {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Integer;
    static getRootAsInteger(bb: flatbuffers.ByteBuffer, obj?: Integer): Integer;
    static getSizePrefixedRootAsInteger(bb: flatbuffers.ByteBuffer, obj?: Integer): Integer;
    value(): number;
    static startInteger(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, value: number): void;
    static endInteger(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createInteger(builder: flatbuffers.Builder, value: number): flatbuffers.Offset;
}
//# sourceMappingURL=integer.d.ts.map