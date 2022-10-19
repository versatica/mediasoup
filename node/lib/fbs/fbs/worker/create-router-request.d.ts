import * as flatbuffers from 'flatbuffers';
export declare class CreateRouterRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CreateRouterRequest;
    static getRootAsCreateRouterRequest(bb: flatbuffers.ByteBuffer, obj?: CreateRouterRequest): CreateRouterRequest;
    static getSizePrefixedRootAsCreateRouterRequest(bb: flatbuffers.ByteBuffer, obj?: CreateRouterRequest): CreateRouterRequest;
    routerId(): string | null;
    routerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCreateRouterRequest(builder: flatbuffers.Builder): void;
    static addRouterId(builder: flatbuffers.Builder, routerIdOffset: flatbuffers.Offset): void;
    static endCreateRouterRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCreateRouterRequest(builder: flatbuffers.Builder, routerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CreateRouterRequestT;
    unpackTo(_o: CreateRouterRequestT): void;
}
export declare class CreateRouterRequestT {
    routerId: string | Uint8Array | null;
    constructor(routerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=create-router-request.d.ts.map