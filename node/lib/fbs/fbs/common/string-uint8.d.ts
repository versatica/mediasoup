import * as flatbuffers from 'flatbuffers';
export declare class StringUint8 {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): StringUint8;
    static getRootAsStringUint8(bb: flatbuffers.ByteBuffer, obj?: StringUint8): StringUint8;
    static getSizePrefixedRootAsStringUint8(bb: flatbuffers.ByteBuffer, obj?: StringUint8): StringUint8;
    key(): string | null;
    key(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    value(): number;
    static startStringUint8(builder: flatbuffers.Builder): void;
    static addKey(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset): void;
    static addValue(builder: flatbuffers.Builder, value: number): void;
    static endStringUint8(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createStringUint8(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset, value: number): flatbuffers.Offset;
    unpack(): StringUint8T;
    unpackTo(_o: StringUint8T): void;
}
export declare class StringUint8T {
    key: string | Uint8Array | null;
    value: number;
    constructor(key?: string | Uint8Array | null, value?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=string-uint8.d.ts.map