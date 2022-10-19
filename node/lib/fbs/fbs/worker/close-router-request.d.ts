import * as flatbuffers from 'flatbuffers';
export declare class CloseRouterRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CloseRouterRequest;
    static getRootAsCloseRouterRequest(bb: flatbuffers.ByteBuffer, obj?: CloseRouterRequest): CloseRouterRequest;
    static getSizePrefixedRootAsCloseRouterRequest(bb: flatbuffers.ByteBuffer, obj?: CloseRouterRequest): CloseRouterRequest;
    routerId(): string | null;
    routerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCloseRouterRequest(builder: flatbuffers.Builder): void;
    static addRouterId(builder: flatbuffers.Builder, routerIdOffset: flatbuffers.Offset): void;
    static endCloseRouterRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCloseRouterRequest(builder: flatbuffers.Builder, routerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CloseRouterRequestT;
    unpackTo(_o: CloseRouterRequestT): void;
}
export declare class CloseRouterRequestT {
    routerId: string | Uint8Array | null;
    constructor(routerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=close-router-request.d.ts.map