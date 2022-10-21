import * as flatbuffers from 'flatbuffers';
import { SctpListener, SctpListenerT } from '../../fbs/transport/sctp-listener';
import { SctpParameters, SctpParametersT } from '../../fbs/transport/sctp-parameters';
export declare class SctpAssociation {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SctpAssociation;
    static getRootAsSctpAssociation(bb: flatbuffers.ByteBuffer, obj?: SctpAssociation): SctpAssociation;
    static getSizePrefixedRootAsSctpAssociation(bb: flatbuffers.ByteBuffer, obj?: SctpAssociation): SctpAssociation;
    sctpParameters(obj?: SctpParameters): SctpParameters | null;
    stcpState(): string | null;
    stcpState(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    stcpListener(obj?: SctpListener): SctpListener | null;
    static startSctpAssociation(builder: flatbuffers.Builder): void;
    static addSctpParameters(builder: flatbuffers.Builder, sctpParametersOffset: flatbuffers.Offset): void;
    static addStcpState(builder: flatbuffers.Builder, stcpStateOffset: flatbuffers.Offset): void;
    static addStcpListener(builder: flatbuffers.Builder, stcpListenerOffset: flatbuffers.Offset): void;
    static endSctpAssociation(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): SctpAssociationT;
    unpackTo(_o: SctpAssociationT): void;
}
export declare class SctpAssociationT {
    sctpParameters: SctpParametersT | null;
    stcpState: string | Uint8Array | null;
    stcpListener: SctpListenerT | null;
    constructor(sctpParameters?: SctpParametersT | null, stcpState?: string | Uint8Array | null, stcpListener?: SctpListenerT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=sctp-association.d.ts.map