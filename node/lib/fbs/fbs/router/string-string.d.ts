import * as flatbuffers from 'flatbuffers';
export declare class StringString {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): StringString;
    static getRootAsStringString(bb: flatbuffers.ByteBuffer, obj?: StringString): StringString;
    static getSizePrefixedRootAsStringString(bb: flatbuffers.ByteBuffer, obj?: StringString): StringString;
    key(): string | null;
    key(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    value(): string | null;
    value(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startStringString(builder: flatbuffers.Builder): void;
    static addKey(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endStringString(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createStringString(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): StringStringT;
    unpackTo(_o: StringStringT): void;
}
export declare class StringStringT {
    key: string | Uint8Array | null;
    value: string | Uint8Array | null;
    constructor(key?: string | Uint8Array | null, value?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=string-string.d.ts.map