import * as flatbuffers from 'flatbuffers';
export declare class IntegerArray {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): IntegerArray;
    static getRootAsIntegerArray(bb: flatbuffers.ByteBuffer, obj?: IntegerArray): IntegerArray;
    static getSizePrefixedRootAsIntegerArray(bb: flatbuffers.ByteBuffer, obj?: IntegerArray): IntegerArray;
    value(index: number): number | null;
    valueLength(): number;
    valueArray(): Int32Array | null;
    static startIntegerArray(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static createValueVector(builder: flatbuffers.Builder, data: number[] | Int32Array): flatbuffers.Offset;
    /**
     * @deprecated This Uint8Array overload will be removed in the future.
     */
    static createValueVector(builder: flatbuffers.Builder, data: number[] | Uint8Array): flatbuffers.Offset;
    static startValueVector(builder: flatbuffers.Builder, numElems: number): void;
    static endIntegerArray(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createIntegerArray(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): IntegerArrayT;
    unpackTo(_o: IntegerArrayT): void;
}
export declare class IntegerArrayT {
    value: (number)[];
    constructor(value?: (number)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=integer-array.d.ts.map