import * as flatbuffers from 'flatbuffers';
import { BaseTransportOptions, BaseTransportOptionsT } from '../../fbs/transport/base-transport-options';
import { WebRtcTransportListen } from '../../fbs/web-rtc-transport/web-rtc-transport-listen';
import { WebRtcTransportListenIndividualT } from '../../fbs/web-rtc-transport/web-rtc-transport-listen-individual';
import { WebRtcTransportListenServerT } from '../../fbs/web-rtc-transport/web-rtc-transport-listen-server';
export declare class WebRtcTransportOptions {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WebRtcTransportOptions;
    static getRootAsWebRtcTransportOptions(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportOptions): WebRtcTransportOptions;
    static getSizePrefixedRootAsWebRtcTransportOptions(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportOptions): WebRtcTransportOptions;
    base(obj?: BaseTransportOptions): BaseTransportOptions | null;
    listenType(): WebRtcTransportListen;
    listen<T extends flatbuffers.Table>(obj: any): any | null;
    enableUdp(): boolean;
    enableTcp(): boolean;
    preferUdp(): boolean;
    preferTcp(): boolean;
    static startWebRtcTransportOptions(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addListenType(builder: flatbuffers.Builder, listenType: WebRtcTransportListen): void;
    static addListen(builder: flatbuffers.Builder, listenOffset: flatbuffers.Offset): void;
    static addEnableUdp(builder: flatbuffers.Builder, enableUdp: boolean): void;
    static addEnableTcp(builder: flatbuffers.Builder, enableTcp: boolean): void;
    static addPreferUdp(builder: flatbuffers.Builder, preferUdp: boolean): void;
    static addPreferTcp(builder: flatbuffers.Builder, preferTcp: boolean): void;
    static endWebRtcTransportOptions(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createWebRtcTransportOptions(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset, listenType: WebRtcTransportListen, listenOffset: flatbuffers.Offset, enableUdp: boolean, enableTcp: boolean, preferUdp: boolean, preferTcp: boolean): flatbuffers.Offset;
    unpack(): WebRtcTransportOptionsT;
    unpackTo(_o: WebRtcTransportOptionsT): void;
}
export declare class WebRtcTransportOptionsT {
    base: BaseTransportOptionsT | null;
    listenType: WebRtcTransportListen;
    listen: WebRtcTransportListenIndividualT | WebRtcTransportListenServerT | null;
    enableUdp: boolean;
    enableTcp: boolean;
    preferUdp: boolean;
    preferTcp: boolean;
    constructor(base?: BaseTransportOptionsT | null, listenType?: WebRtcTransportListen, listen?: WebRtcTransportListenIndividualT | WebRtcTransportListenServerT | null, enableUdp?: boolean, enableTcp?: boolean, preferUdp?: boolean, preferTcp?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=web-rtc-transport-options.d.ts.map