import * as flatbuffers from 'flatbuffers';
import { NumSctpStreams, NumSctpStreamsT } from '../../fbs/router/num-sctp-streams';
export declare class CreateWebRtcTransportWithServerRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CreateWebRtcTransportWithServerRequest;
    static getRootAsCreateWebRtcTransportWithServerRequest(bb: flatbuffers.ByteBuffer, obj?: CreateWebRtcTransportWithServerRequest): CreateWebRtcTransportWithServerRequest;
    static getSizePrefixedRootAsCreateWebRtcTransportWithServerRequest(bb: flatbuffers.ByteBuffer, obj?: CreateWebRtcTransportWithServerRequest): CreateWebRtcTransportWithServerRequest;
    transportId(): string | null;
    transportId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    webRtcServerId(): string | null;
    webRtcServerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    enableUdp(): boolean;
    enableTcp(): boolean;
    preferUdp(): boolean;
    preferTcp(): boolean;
    initialAvailableOutgoingBitrate(): number;
    enableSctp(): boolean;
    numSctpStreams(obj?: NumSctpStreams): NumSctpStreams | null;
    maxSctpMessageSize(): number;
    sctpSendBufferSize(): number;
    isDataChannel(): boolean;
    static startCreateWebRtcTransportWithServerRequest(builder: flatbuffers.Builder): void;
    static addTransportId(builder: flatbuffers.Builder, transportIdOffset: flatbuffers.Offset): void;
    static addWebRtcServerId(builder: flatbuffers.Builder, webRtcServerIdOffset: flatbuffers.Offset): void;
    static addEnableUdp(builder: flatbuffers.Builder, enableUdp: boolean): void;
    static addEnableTcp(builder: flatbuffers.Builder, enableTcp: boolean): void;
    static addPreferUdp(builder: flatbuffers.Builder, preferUdp: boolean): void;
    static addPreferTcp(builder: flatbuffers.Builder, preferTcp: boolean): void;
    static addInitialAvailableOutgoingBitrate(builder: flatbuffers.Builder, initialAvailableOutgoingBitrate: number): void;
    static addEnableSctp(builder: flatbuffers.Builder, enableSctp: boolean): void;
    static addNumSctpStreams(builder: flatbuffers.Builder, numSctpStreamsOffset: flatbuffers.Offset): void;
    static addMaxSctpMessageSize(builder: flatbuffers.Builder, maxSctpMessageSize: number): void;
    static addSctpSendBufferSize(builder: flatbuffers.Builder, sctpSendBufferSize: number): void;
    static addIsDataChannel(builder: flatbuffers.Builder, isDataChannel: boolean): void;
    static endCreateWebRtcTransportWithServerRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): CreateWebRtcTransportWithServerRequestT;
    unpackTo(_o: CreateWebRtcTransportWithServerRequestT): void;
}
export declare class CreateWebRtcTransportWithServerRequestT {
    transportId: string | Uint8Array | null;
    webRtcServerId: string | Uint8Array | null;
    enableUdp: boolean;
    enableTcp: boolean;
    preferUdp: boolean;
    preferTcp: boolean;
    initialAvailableOutgoingBitrate: number;
    enableSctp: boolean;
    numSctpStreams: NumSctpStreamsT | null;
    maxSctpMessageSize: number;
    sctpSendBufferSize: number;
    isDataChannel: boolean;
    constructor(transportId?: string | Uint8Array | null, webRtcServerId?: string | Uint8Array | null, enableUdp?: boolean, enableTcp?: boolean, preferUdp?: boolean, preferTcp?: boolean, initialAvailableOutgoingBitrate?: number, enableSctp?: boolean, numSctpStreams?: NumSctpStreamsT | null, maxSctpMessageSize?: number, sctpSendBufferSize?: number, isDataChannel?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=create-web-rtc-transport-with-server-request.d.ts.map