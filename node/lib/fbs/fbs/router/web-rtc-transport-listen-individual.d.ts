import * as flatbuffers from 'flatbuffers';
import { TransportListenIp, TransportListenIpT } from '../../fbs/router/transport-listen-ip';
export declare class WebRtcTransportListenIndividual {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WebRtcTransportListenIndividual;
    static getRootAsWebRtcTransportListenIndividual(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportListenIndividual): WebRtcTransportListenIndividual;
    static getSizePrefixedRootAsWebRtcTransportListenIndividual(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportListenIndividual): WebRtcTransportListenIndividual;
    listenIps(index: number, obj?: TransportListenIp): TransportListenIp | null;
    listenIpsLength(): number;
    port(): number;
    static startWebRtcTransportListenIndividual(builder: flatbuffers.Builder): void;
    static addListenIps(builder: flatbuffers.Builder, listenIpsOffset: flatbuffers.Offset): void;
    static createListenIpsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startListenIpsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addPort(builder: flatbuffers.Builder, port: number): void;
    static endWebRtcTransportListenIndividual(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createWebRtcTransportListenIndividual(builder: flatbuffers.Builder, listenIpsOffset: flatbuffers.Offset, port: number): flatbuffers.Offset;
    unpack(): WebRtcTransportListenIndividualT;
    unpackTo(_o: WebRtcTransportListenIndividualT): void;
}
export declare class WebRtcTransportListenIndividualT {
    listenIps: (TransportListenIpT)[];
    port: number;
    constructor(listenIps?: (TransportListenIpT)[], port?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=web-rtc-transport-listen-individual.d.ts.map