import * as flatbuffers from 'flatbuffers';
import { NumSctpStreams, NumSctpStreamsT } from '../../fbs/router/num-sctp-streams';
import { WebRtcTransportListen } from '../../fbs/router/web-rtc-transport-listen';
import { WebRtcTransportListenIndividualT } from '../../fbs/router/web-rtc-transport-listen-individual';
import { WebRtcTransportListenServerT } from '../../fbs/router/web-rtc-transport-listen-server';
export declare class WebRtcTransportOptions {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WebRtcTransportOptions;
    static getRootAsWebRtcTransportOptions(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportOptions): WebRtcTransportOptions;
    static getSizePrefixedRootAsWebRtcTransportOptions(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportOptions): WebRtcTransportOptions;
    listenType(): WebRtcTransportListen;
    listen<T extends flatbuffers.Table>(obj: any): any | null;
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
    static startWebRtcTransportOptions(builder: flatbuffers.Builder): void;
    static addListenType(builder: flatbuffers.Builder, listenType: WebRtcTransportListen): void;
    static addListen(builder: flatbuffers.Builder, listenOffset: flatbuffers.Offset): void;
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
    static endWebRtcTransportOptions(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): WebRtcTransportOptionsT;
    unpackTo(_o: WebRtcTransportOptionsT): void;
}
export declare class WebRtcTransportOptionsT {
    listenType: WebRtcTransportListen;
    listen: WebRtcTransportListenIndividualT | WebRtcTransportListenServerT | null;
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
    constructor(listenType?: WebRtcTransportListen, listen?: WebRtcTransportListenIndividualT | WebRtcTransportListenServerT | null, enableUdp?: boolean, enableTcp?: boolean, preferUdp?: boolean, preferTcp?: boolean, initialAvailableOutgoingBitrate?: number, enableSctp?: boolean, numSctpStreams?: NumSctpStreamsT | null, maxSctpMessageSize?: number, sctpSendBufferSize?: number, isDataChannel?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=web-rtc-transport-options.d.ts.map