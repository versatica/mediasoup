import * as flatbuffers from 'flatbuffers';
export declare class Uint32String {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Uint32String;
    static getRootAsUint32String(bb: flatbuffers.ByteBuffer, obj?: Uint32String): Uint32String;
    static getSizePrefixedRootAsUint32String(bb: flatbuffers.ByteBuffer, obj?: Uint32String): Uint32String;
    key(): number;
    value(): string | null;
    value(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startUint32String(builder: flatbuffers.Builder): void;
    static addKey(builder: flatbuffers.Builder, key: number): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endUint32String(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createUint32String(builder: flatbuffers.Builder, key: number, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): Uint32StringT;
    unpackTo(_o: Uint32StringT): void;
}
export declare class Uint32StringT {
    key: number;
    value: string | Uint8Array | null;
    constructor(key?: number, value?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=uint32string.d.ts.map