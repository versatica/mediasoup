import * as flatbuffers from 'flatbuffers';
import { TransportProtocol } from '../../f-b-s/worker/transport-protocol';
export declare class WebRtcServerListenInfo {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WebRtcServerListenInfo;
    static getRootAsWebRtcServerListenInfo(bb: flatbuffers.ByteBuffer, obj?: WebRtcServerListenInfo): WebRtcServerListenInfo;
    static getSizePrefixedRootAsWebRtcServerListenInfo(bb: flatbuffers.ByteBuffer, obj?: WebRtcServerListenInfo): WebRtcServerListenInfo;
    protocol(): TransportProtocol;
    ip(): string | null;
    ip(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    announcedIp(): string | null;
    announcedIp(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    port(): number;
    static startWebRtcServerListenInfo(builder: flatbuffers.Builder): void;
    static addProtocol(builder: flatbuffers.Builder, protocol: TransportProtocol): void;
    static addIp(builder: flatbuffers.Builder, ipOffset: flatbuffers.Offset): void;
    static addAnnouncedIp(builder: flatbuffers.Builder, announcedIpOffset: flatbuffers.Offset): void;
    static addPort(builder: flatbuffers.Builder, port: number): void;
    static endWebRtcServerListenInfo(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createWebRtcServerListenInfo(builder: flatbuffers.Builder, protocol: TransportProtocol, ipOffset: flatbuffers.Offset, announcedIpOffset: flatbuffers.Offset, port: number): flatbuffers.Offset;
    unpack(): WebRtcServerListenInfoT;
    unpackTo(_o: WebRtcServerListenInfoT): void;
}
export declare class WebRtcServerListenInfoT {
    protocol: TransportProtocol;
    ip: string | Uint8Array | null;
    announcedIp: string | Uint8Array | null;
    port: number;
    constructor(protocol?: TransportProtocol, ip?: string | Uint8Array | null, announcedIp?: string | Uint8Array | null, port?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=web-rtc-server-listen-info.d.ts.map