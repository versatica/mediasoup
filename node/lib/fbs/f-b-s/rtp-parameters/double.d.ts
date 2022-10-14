import * as flatbuffers from 'flatbuffers';
export declare class Double {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Double;
    static getRootAsDouble(bb: flatbuffers.ByteBuffer, obj?: Double): Double;
    static getSizePrefixedRootAsDouble(bb: flatbuffers.ByteBuffer, obj?: Double): Double;
    value(): number;
    static startDouble(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, value: number): void;
    static endDouble(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDouble(builder: flatbuffers.Builder, value: number): flatbuffers.Offset;
}
//# sourceMappingURL=double.d.ts.map