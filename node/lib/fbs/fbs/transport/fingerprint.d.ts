import * as flatbuffers from 'flatbuffers';
export declare class Fingerprint {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Fingerprint;
    static getRootAsFingerprint(bb: flatbuffers.ByteBuffer, obj?: Fingerprint): Fingerprint;
    static getSizePrefixedRootAsFingerprint(bb: flatbuffers.ByteBuffer, obj?: Fingerprint): Fingerprint;
    algorithm(): string | null;
    algorithm(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    value(): string | null;
    value(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startFingerprint(builder: flatbuffers.Builder): void;
    static addAlgorithm(builder: flatbuffers.Builder, algorithmOffset: flatbuffers.Offset): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endFingerprint(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createFingerprint(builder: flatbuffers.Builder, algorithmOffset: flatbuffers.Offset, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): FingerprintT;
    unpackTo(_o: FingerprintT): void;
}
export declare class FingerprintT {
    algorithm: string | Uint8Array | null;
    value: string | Uint8Array | null;
    constructor(algorithm?: string | Uint8Array | null, value?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=fingerprint.d.ts.map