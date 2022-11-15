import * as flatbuffers from 'flatbuffers';
export declare class OptionalInt16 {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): OptionalInt16;
    static getRootAsOptionalInt16(bb: flatbuffers.ByteBuffer, obj?: OptionalInt16): OptionalInt16;
    static getSizePrefixedRootAsOptionalInt16(bb: flatbuffers.ByteBuffer, obj?: OptionalInt16): OptionalInt16;
    value(): number;
    static startOptionalInt16(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, value: number): void;
    static endOptionalInt16(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createOptionalInt16(builder: flatbuffers.Builder, value: number): flatbuffers.Offset;
    unpack(): OptionalInt16T;
    unpackTo(_o: OptionalInt16T): void;
}
export declare class OptionalInt16T {
    value: number;
    constructor(value?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=optional-int16.d.ts.map