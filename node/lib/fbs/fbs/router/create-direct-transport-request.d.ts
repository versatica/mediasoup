import * as flatbuffers from 'flatbuffers';
import { DirectTransportOptions, DirectTransportOptionsT } from '../../fbs/direct-transport/direct-transport-options';
export declare class CreateDirectTransportRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CreateDirectTransportRequest;
    static getRootAsCreateDirectTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CreateDirectTransportRequest): CreateDirectTransportRequest;
    static getSizePrefixedRootAsCreateDirectTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CreateDirectTransportRequest): CreateDirectTransportRequest;
    transportId(): string | null;
    transportId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    options(obj?: DirectTransportOptions): DirectTransportOptions | null;
    static startCreateDirectTransportRequest(builder: flatbuffers.Builder): void;
    static addTransportId(builder: flatbuffers.Builder, transportIdOffset: flatbuffers.Offset): void;
    static addOptions(builder: flatbuffers.Builder, optionsOffset: flatbuffers.Offset): void;
    static endCreateDirectTransportRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): CreateDirectTransportRequestT;
    unpackTo(_o: CreateDirectTransportRequestT): void;
}
export declare class CreateDirectTransportRequestT {
    transportId: string | Uint8Array | null;
    options: DirectTransportOptionsT | null;
    constructor(transportId?: string | Uint8Array | null, options?: DirectTransportOptionsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=create-direct-transport-request.d.ts.map