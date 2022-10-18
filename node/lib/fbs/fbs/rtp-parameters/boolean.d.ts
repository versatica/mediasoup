import * as flatbuffers from 'flatbuffers';
export declare class Boolean {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Boolean;
    static getRootAsBoolean(bb: flatbuffers.ByteBuffer, obj?: Boolean): Boolean;
    static getSizePrefixedRootAsBoolean(bb: flatbuffers.ByteBuffer, obj?: Boolean): Boolean;
    value(): number;
    static startBoolean(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, value: number): void;
    static endBoolean(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createBoolean(builder: flatbuffers.Builder, value: number): flatbuffers.Offset;
    unpack(): BooleanT;
    unpackTo(_o: BooleanT): void;
}
export declare class BooleanT {
    value: number;
    constructor(value?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=boolean.d.ts.map