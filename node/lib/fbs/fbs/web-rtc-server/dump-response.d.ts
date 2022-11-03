import * as flatbuffers from 'flatbuffers';
import { IceUserNameFragment, IceUserNameFragmentT } from '../../fbs/web-rtc-server/ice-user-name-fragment';
import { IpPort, IpPortT } from '../../fbs/web-rtc-server/ip-port';
import { TupleHash, TupleHashT } from '../../fbs/web-rtc-server/tuple-hash';
export declare class DumpResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DumpResponse;
    static getRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    static getSizePrefixedRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    id(): string | null;
    id(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    udpSockets(index: number, obj?: IpPort): IpPort | null;
    udpSocketsLength(): number;
    tcpServers(index: number, obj?: IpPort): IpPort | null;
    tcpServersLength(): number;
    webRtcTransportIds(index: number): string;
    webRtcTransportIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    webRtcTransportIdsLength(): number;
    localIceUsernameFragments(index: number, obj?: IceUserNameFragment): IceUserNameFragment | null;
    localIceUsernameFragmentsLength(): number;
    tupleHashes(index: number, obj?: TupleHash): TupleHash | null;
    tupleHashesLength(): number;
    static startDumpResponse(builder: flatbuffers.Builder): void;
    static addId(builder: flatbuffers.Builder, idOffset: flatbuffers.Offset): void;
    static addUdpSockets(builder: flatbuffers.Builder, udpSocketsOffset: flatbuffers.Offset): void;
    static createUdpSocketsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startUdpSocketsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addTcpServers(builder: flatbuffers.Builder, tcpServersOffset: flatbuffers.Offset): void;
    static createTcpServersVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startTcpServersVector(builder: flatbuffers.Builder, numElems: number): void;
    static addWebRtcTransportIds(builder: flatbuffers.Builder, webRtcTransportIdsOffset: flatbuffers.Offset): void;
    static createWebRtcTransportIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startWebRtcTransportIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addLocalIceUsernameFragments(builder: flatbuffers.Builder, localIceUsernameFragmentsOffset: flatbuffers.Offset): void;
    static createLocalIceUsernameFragmentsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startLocalIceUsernameFragmentsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addTupleHashes(builder: flatbuffers.Builder, tupleHashesOffset: flatbuffers.Offset): void;
    static createTupleHashesVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startTupleHashesVector(builder: flatbuffers.Builder, numElems: number): void;
    static endDumpResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDumpResponse(builder: flatbuffers.Builder, idOffset: flatbuffers.Offset, udpSocketsOffset: flatbuffers.Offset, tcpServersOffset: flatbuffers.Offset, webRtcTransportIdsOffset: flatbuffers.Offset, localIceUsernameFragmentsOffset: flatbuffers.Offset, tupleHashesOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): DumpResponseT;
    unpackTo(_o: DumpResponseT): void;
}
export declare class DumpResponseT {
    id: string | Uint8Array | null;
    udpSockets: (IpPortT)[];
    tcpServers: (IpPortT)[];
    webRtcTransportIds: (string)[];
    localIceUsernameFragments: (IceUserNameFragmentT)[];
    tupleHashes: (TupleHashT)[];
    constructor(id?: string | Uint8Array | null, udpSockets?: (IpPortT)[], tcpServers?: (IpPortT)[], webRtcTransportIds?: (string)[], localIceUsernameFragments?: (IceUserNameFragmentT)[], tupleHashes?: (TupleHashT)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump-response.d.ts.map