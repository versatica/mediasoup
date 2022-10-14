import * as flatbuffers from 'flatbuffers';
export declare class IntArray {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): IntArray;
    static getRootAsIntArray(bb: flatbuffers.ByteBuffer, obj?: IntArray): IntArray;
    static getSizePrefixedRootAsIntArray(bb: flatbuffers.ByteBuffer, obj?: IntArray): IntArray;
    value(index: number): number | null;
    valueLength(): number;
    valueArray(): Uint32Array | null;
    static startIntArray(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static createValueVector(builder: flatbuffers.Builder, data: number[] | Uint32Array): flatbuffers.Offset;
    /**
     * @deprecated This Uint8Array overload will be removed in the future.
     */
    static createValueVector(builder: flatbuffers.Builder, data: number[] | Uint8Array): flatbuffers.Offset;
    static startValueVector(builder: flatbuffers.Builder, numElems: number): void;
    static endIntArray(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createIntArray(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
}
//# sourceMappingURL=int-array.d.ts.map