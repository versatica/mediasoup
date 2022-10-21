import * as flatbuffers from 'flatbuffers';
import { Fingerprint, FingerprintT } from '../../fbs/transport/fingerprint';
export declare class DtlsParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DtlsParameters;
    static getRootAsDtlsParameters(bb: flatbuffers.ByteBuffer, obj?: DtlsParameters): DtlsParameters;
    static getSizePrefixedRootAsDtlsParameters(bb: flatbuffers.ByteBuffer, obj?: DtlsParameters): DtlsParameters;
    fingerprint(index: number, obj?: Fingerprint): Fingerprint | null;
    fingerprintLength(): number;
    role(): string | null;
    role(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    dtlsState(): string | null;
    dtlsState(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startDtlsParameters(builder: flatbuffers.Builder): void;
    static addFingerprint(builder: flatbuffers.Builder, fingerprintOffset: flatbuffers.Offset): void;
    static createFingerprintVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startFingerprintVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRole(builder: flatbuffers.Builder, roleOffset: flatbuffers.Offset): void;
    static addDtlsState(builder: flatbuffers.Builder, dtlsStateOffset: flatbuffers.Offset): void;
    static endDtlsParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDtlsParameters(builder: flatbuffers.Builder, fingerprintOffset: flatbuffers.Offset, roleOffset: flatbuffers.Offset, dtlsStateOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): DtlsParametersT;
    unpackTo(_o: DtlsParametersT): void;
}
export declare class DtlsParametersT {
    fingerprint: (FingerprintT)[];
    role: string | Uint8Array | null;
    dtlsState: string | Uint8Array | null;
    constructor(fingerprint?: (FingerprintT)[], role?: string | Uint8Array | null, dtlsState?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dtls-parameters.d.ts.map