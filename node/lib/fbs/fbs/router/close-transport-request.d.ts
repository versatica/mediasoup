import * as flatbuffers from 'flatbuffers';
export declare class CloseTransportRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CloseTransportRequest;
    static getRootAsCloseTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CloseTransportRequest): CloseTransportRequest;
    static getSizePrefixedRootAsCloseTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CloseTransportRequest): CloseTransportRequest;
    transportId(): string | null;
    transportId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCloseTransportRequest(builder: flatbuffers.Builder): void;
    static addTransportId(builder: flatbuffers.Builder, transportIdOffset: flatbuffers.Offset): void;
    static endCloseTransportRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCloseTransportRequest(builder: flatbuffers.Builder, transportIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CloseTransportRequestT;
    unpackTo(_o: CloseTransportRequestT): void;
}
export declare class CloseTransportRequestT {
    transportId: string | Uint8Array | null;
    constructor(transportId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=close-transport-request.d.ts.map