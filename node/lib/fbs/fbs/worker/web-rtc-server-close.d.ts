import * as flatbuffers from 'flatbuffers';
export declare class WebRtcServerClose {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WebRtcServerClose;
    static getRootAsWebRtcServerClose(bb: flatbuffers.ByteBuffer, obj?: WebRtcServerClose): WebRtcServerClose;
    static getSizePrefixedRootAsWebRtcServerClose(bb: flatbuffers.ByteBuffer, obj?: WebRtcServerClose): WebRtcServerClose;
    webRtcServerId(): string | null;
    webRtcServerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startWebRtcServerClose(builder: flatbuffers.Builder): void;
    static addWebRtcServerId(builder: flatbuffers.Builder, webRtcServerIdOffset: flatbuffers.Offset): void;
    static endWebRtcServerClose(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createWebRtcServerClose(builder: flatbuffers.Builder, webRtcServerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): WebRtcServerCloseT;
    unpackTo(_o: WebRtcServerCloseT): void;
}
export declare class WebRtcServerCloseT {
    webRtcServerId: string | Uint8Array | null;
    constructor(webRtcServerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=web-rtc-server-close.d.ts.map