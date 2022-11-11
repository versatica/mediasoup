import * as flatbuffers from 'flatbuffers';
export declare class CloseRtpObserverRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CloseRtpObserverRequest;
    static getRootAsCloseRtpObserverRequest(bb: flatbuffers.ByteBuffer, obj?: CloseRtpObserverRequest): CloseRtpObserverRequest;
    static getSizePrefixedRootAsCloseRtpObserverRequest(bb: flatbuffers.ByteBuffer, obj?: CloseRtpObserverRequest): CloseRtpObserverRequest;
    rtpObserverId(): string | null;
    rtpObserverId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCloseRtpObserverRequest(builder: flatbuffers.Builder): void;
    static addRtpObserverId(builder: flatbuffers.Builder, rtpObserverIdOffset: flatbuffers.Offset): void;
    static endCloseRtpObserverRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCloseRtpObserverRequest(builder: flatbuffers.Builder, rtpObserverIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CloseRtpObserverRequestT;
    unpackTo(_o: CloseRtpObserverRequestT): void;
}
export declare class CloseRtpObserverRequestT {
    rtpObserverId: string | Uint8Array | null;
    constructor(rtpObserverId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=close-rtp-observer-request.d.ts.map