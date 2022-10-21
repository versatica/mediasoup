import * as flatbuffers from 'flatbuffers';
export declare class StringStringMap {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): StringStringMap;
    static getRootAsStringStringMap(bb: flatbuffers.ByteBuffer, obj?: StringStringMap): StringStringMap;
    static getSizePrefixedRootAsStringStringMap(bb: flatbuffers.ByteBuffer, obj?: StringStringMap): StringStringMap;
    key(): string | null;
    key(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    value(): string | null;
    value(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startStringStringMap(builder: flatbuffers.Builder): void;
    static addKey(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endStringStringMap(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createStringStringMap(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): StringStringMapT;
    unpackTo(_o: StringStringMapT): void;
}
export declare class StringStringMapT {
    key: string | Uint8Array | null;
    value: string | Uint8Array | null;
    constructor(key?: string | Uint8Array | null, value?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=string-string-map.d.ts.map