import * as flatbuffers from 'flatbuffers';
export declare class Uint16String {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Uint16String;
    static getRootAsUint16String(bb: flatbuffers.ByteBuffer, obj?: Uint16String): Uint16String;
    static getSizePrefixedRootAsUint16String(bb: flatbuffers.ByteBuffer, obj?: Uint16String): Uint16String;
    key(): number;
    value(): string | null;
    value(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startUint16String(builder: flatbuffers.Builder): void;
    static addKey(builder: flatbuffers.Builder, key: number): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endUint16String(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createUint16String(builder: flatbuffers.Builder, key: number, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): Uint16StringT;
    unpackTo(_o: Uint16StringT): void;
}
export declare class Uint16StringT {
    key: number;
    value: string | Uint8Array | null;
    constructor(key?: number, value?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=uint16string.d.ts.map