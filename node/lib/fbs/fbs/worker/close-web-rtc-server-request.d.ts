import * as flatbuffers from 'flatbuffers';
export declare class CloseWebRtcServerRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CloseWebRtcServerRequest;
    static getRootAsCloseWebRtcServerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseWebRtcServerRequest): CloseWebRtcServerRequest;
    static getSizePrefixedRootAsCloseWebRtcServerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseWebRtcServerRequest): CloseWebRtcServerRequest;
    webRtcServerId(): string | null;
    webRtcServerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCloseWebRtcServerRequest(builder: flatbuffers.Builder): void;
    static addWebRtcServerId(builder: flatbuffers.Builder, webRtcServerIdOffset: flatbuffers.Offset): void;
    static endCloseWebRtcServerRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCloseWebRtcServerRequest(builder: flatbuffers.Builder, webRtcServerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CloseWebRtcServerRequestT;
    unpackTo(_o: CloseWebRtcServerRequestT): void;
}
export declare class CloseWebRtcServerRequestT {
    webRtcServerId: string | Uint8Array | null;
    constructor(webRtcServerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=close-web-rtc-server-request.d.ts.map