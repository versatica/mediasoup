import * as flatbuffers from 'flatbuffers';
export declare class StringStringArray {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): StringStringArray;
    static getRootAsStringStringArray(bb: flatbuffers.ByteBuffer, obj?: StringStringArray): StringStringArray;
    static getSizePrefixedRootAsStringStringArray(bb: flatbuffers.ByteBuffer, obj?: StringStringArray): StringStringArray;
    key(): string | null;
    key(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    values(index: number): string;
    values(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    valuesLength(): number;
    static startStringStringArray(builder: flatbuffers.Builder): void;
    static addKey(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset): void;
    static addValues(builder: flatbuffers.Builder, valuesOffset: flatbuffers.Offset): void;
    static createValuesVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startValuesVector(builder: flatbuffers.Builder, numElems: number): void;
    static endStringStringArray(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createStringStringArray(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset, valuesOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): StringStringArrayT;
    unpackTo(_o: StringStringArrayT): void;
}
export declare class StringStringArrayT {
    key: string | Uint8Array | null;
    values: (string)[];
    constructor(key?: string | Uint8Array | null, values?: (string)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=string-string-array.d.ts.map