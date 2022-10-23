import * as flatbuffers from 'flatbuffers';
import { WebRtcTransportOptions, WebRtcTransportOptionsT } from '../../fbs/web-rtc-transport/web-rtc-transport-options';
export declare class CreateWebRtcTransportRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CreateWebRtcTransportRequest;
    static getRootAsCreateWebRtcTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CreateWebRtcTransportRequest): CreateWebRtcTransportRequest;
    static getSizePrefixedRootAsCreateWebRtcTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CreateWebRtcTransportRequest): CreateWebRtcTransportRequest;
    transportId(): string | null;
    transportId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    options(obj?: WebRtcTransportOptions): WebRtcTransportOptions | null;
    static startCreateWebRtcTransportRequest(builder: flatbuffers.Builder): void;
    static addTransportId(builder: flatbuffers.Builder, transportIdOffset: flatbuffers.Offset): void;
    static addOptions(builder: flatbuffers.Builder, optionsOffset: flatbuffers.Offset): void;
    static endCreateWebRtcTransportRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): CreateWebRtcTransportRequestT;
    unpackTo(_o: CreateWebRtcTransportRequestT): void;
}
export declare class CreateWebRtcTransportRequestT {
    transportId: string | Uint8Array | null;
    options: WebRtcTransportOptionsT | null;
    constructor(transportId?: string | Uint8Array | null, options?: WebRtcTransportOptionsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=create-web-rtc-transport-request.d.ts.map