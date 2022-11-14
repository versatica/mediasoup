import * as flatbuffers from 'flatbuffers';
export declare class OptionalUint16 {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): OptionalUint16;
    static getRootAsOptionalUint16(bb: flatbuffers.ByteBuffer, obj?: OptionalUint16): OptionalUint16;
    static getSizePrefixedRootAsOptionalUint16(bb: flatbuffers.ByteBuffer, obj?: OptionalUint16): OptionalUint16;
    value(): number;
    static startOptionalUint16(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, value: number): void;
    static endOptionalUint16(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createOptionalUint16(builder: flatbuffers.Builder, value: number): flatbuffers.Offset;
    unpack(): OptionalUint16T;
    unpackTo(_o: OptionalUint16T): void;
}
export declare class OptionalUint16T {
    value: number;
    constructor(value?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=optional-uint16.d.ts.map