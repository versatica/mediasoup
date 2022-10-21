import * as flatbuffers from 'flatbuffers';
export declare class StringUint8Map {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): StringUint8Map;
    static getRootAsStringUint8Map(bb: flatbuffers.ByteBuffer, obj?: StringUint8Map): StringUint8Map;
    static getSizePrefixedRootAsStringUint8Map(bb: flatbuffers.ByteBuffer, obj?: StringUint8Map): StringUint8Map;
    key(): string | null;
    key(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    value(): number;
    static startStringUint8Map(builder: flatbuffers.Builder): void;
    static addKey(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset): void;
    static addValue(builder: flatbuffers.Builder, value: number): void;
    static endStringUint8Map(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createStringUint8Map(builder: flatbuffers.Builder, keyOffset: flatbuffers.Offset, value: number): flatbuffers.Offset;
    unpack(): StringUint8MapT;
    unpackTo(_o: StringUint8MapT): void;
}
export declare class StringUint8MapT {
    key: string | Uint8Array | null;
    value: number;
    constructor(key?: string | Uint8Array | null, value?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=string-uint8map.d.ts.map