import * as flatbuffers from 'flatbuffers';
import { DtlsParameters, DtlsParametersT } from '../../fbs/transport/dtls-parameters';
import { IceCandidate, IceCandidateT } from '../../fbs/transport/ice-candidate';
import { IceParameters, IceParametersT } from '../../fbs/transport/ice-parameters';
import { TransportDump, TransportDumpT } from '../../fbs/transport/transport-dump';
import { Tuple, TupleT } from '../../fbs/transport/tuple';
export declare class WebRtcTransportDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WebRtcTransportDump;
    static getRootAsWebRtcTransportDump(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportDump): WebRtcTransportDump;
    static getSizePrefixedRootAsWebRtcTransportDump(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportDump): WebRtcTransportDump;
    base(obj?: TransportDump): TransportDump | null;
    iceRole(): string | null;
    iceRole(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    iceParameters(obj?: IceParameters): IceParameters | null;
    iceCandidates(index: number, obj?: IceCandidate): IceCandidate | null;
    iceCandidatesLength(): number;
    iceState(): string | null;
    iceState(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    iceSelectedTuple(obj?: Tuple): Tuple | null;
    dtlsParameters(obj?: DtlsParameters): DtlsParameters | null;
    dtlsState(): string | null;
    dtlsState(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startWebRtcTransportDump(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addIceRole(builder: flatbuffers.Builder, iceRoleOffset: flatbuffers.Offset): void;
    static addIceParameters(builder: flatbuffers.Builder, iceParametersOffset: flatbuffers.Offset): void;
    static addIceCandidates(builder: flatbuffers.Builder, iceCandidatesOffset: flatbuffers.Offset): void;
    static createIceCandidatesVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startIceCandidatesVector(builder: flatbuffers.Builder, numElems: number): void;
    static addIceState(builder: flatbuffers.Builder, iceStateOffset: flatbuffers.Offset): void;
    static addIceSelectedTuple(builder: flatbuffers.Builder, iceSelectedTupleOffset: flatbuffers.Offset): void;
    static addDtlsParameters(builder: flatbuffers.Builder, dtlsParametersOffset: flatbuffers.Offset): void;
    static addDtlsState(builder: flatbuffers.Builder, dtlsStateOffset: flatbuffers.Offset): void;
    static endWebRtcTransportDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): WebRtcTransportDumpT;
    unpackTo(_o: WebRtcTransportDumpT): void;
}
export declare class WebRtcTransportDumpT {
    base: TransportDumpT | null;
    iceRole: string | Uint8Array | null;
    iceParameters: IceParametersT | null;
    iceCandidates: (IceCandidateT)[];
    iceState: string | Uint8Array | null;
    iceSelectedTuple: TupleT | null;
    dtlsParameters: DtlsParametersT | null;
    dtlsState: string | Uint8Array | null;
    constructor(base?: TransportDumpT | null, iceRole?: string | Uint8Array | null, iceParameters?: IceParametersT | null, iceCandidates?: (IceCandidateT)[], iceState?: string | Uint8Array | null, iceSelectedTuple?: TupleT | null, dtlsParameters?: DtlsParametersT | null, dtlsState?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=web-rtc-transport-dump.d.ts.map