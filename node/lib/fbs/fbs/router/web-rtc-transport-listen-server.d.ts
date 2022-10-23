import * as flatbuffers from 'flatbuffers';
export declare class WebRtcTransportListenServer {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WebRtcTransportListenServer;
    static getRootAsWebRtcTransportListenServer(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportListenServer): WebRtcTransportListenServer;
    static getSizePrefixedRootAsWebRtcTransportListenServer(bb: flatbuffers.ByteBuffer, obj?: WebRtcTransportListenServer): WebRtcTransportListenServer;
    webRtcServerId(): string | null;
    webRtcServerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startWebRtcTransportListenServer(builder: flatbuffers.Builder): void;
    static addWebRtcServerId(builder: flatbuffers.Builder, webRtcServerIdOffset: flatbuffers.Offset): void;
    static endWebRtcTransportListenServer(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createWebRtcTransportListenServer(builder: flatbuffers.Builder, webRtcServerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): WebRtcTransportListenServerT;
    unpackTo(_o: WebRtcTransportListenServerT): void;
}
export declare class WebRtcTransportListenServerT {
    webRtcServerId: string | Uint8Array | null;
    constructor(webRtcServerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=web-rtc-transport-listen-server.d.ts.map