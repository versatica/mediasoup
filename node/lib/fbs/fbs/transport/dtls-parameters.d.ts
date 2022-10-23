import * as flatbuffers from 'flatbuffers';
import { Fingerprint, FingerprintT } from '../../fbs/transport/fingerprint';
export declare class DtlsParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DtlsParameters;
    static getRootAsDtlsParameters(bb: flatbuffers.ByteBuffer, obj?: DtlsParameters): DtlsParameters;
    static getSizePrefixedRootAsDtlsParameters(bb: flatbuffers.ByteBuffer, obj?: DtlsParameters): DtlsParameters;
    fingerprints(index: number, obj?: Fingerprint): Fingerprint | null;
    fingerprintsLength(): number;
    role(): string | null;
    role(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startDtlsParameters(builder: flatbuffers.Builder): void;
    static addFingerprints(builder: flatbuffers.Builder, fingerprintsOffset: flatbuffers.Offset): void;
    static createFingerprintsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startFingerprintsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRole(builder: flatbuffers.Builder, roleOffset: flatbuffers.Offset): void;
    static endDtlsParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDtlsParameters(builder: flatbuffers.Builder, fingerprintsOffset: flatbuffers.Offset, roleOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): DtlsParametersT;
    unpackTo(_o: DtlsParametersT): void;
}
export declare class DtlsParametersT {
    fingerprints: (FingerprintT)[];
    role: string | Uint8Array | null;
    constructor(fingerprints?: (FingerprintT)[], role?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dtls-parameters.d.ts.map