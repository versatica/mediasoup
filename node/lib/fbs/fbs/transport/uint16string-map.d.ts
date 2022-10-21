import * as flatbuffers from 'flatbuffers';
export declare class Uint16StringMap {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Uint16StringMap;
    static getRootAsUint16StringMap(bb: flatbuffers.ByteBuffer, obj?: Uint16StringMap): Uint16StringMap;
    static getSizePrefixedRootAsUint16StringMap(bb: flatbuffers.ByteBuffer, obj?: Uint16StringMap): Uint16StringMap;
    key(): number;
    value(): string | null;
    value(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startUint16StringMap(builder: flatbuffers.Builder): void;
    static addKey(builder: flatbuffers.Builder, key: number): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endUint16StringMap(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createUint16StringMap(builder: flatbuffers.Builder, key: number, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): Uint16StringMapT;
    unpackTo(_o: Uint16StringMapT): void;
}
export declare class Uint16StringMapT {
    key: number;
    value: string | Uint8Array | null;
    constructor(key?: number, value?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=uint16string-map.d.ts.map