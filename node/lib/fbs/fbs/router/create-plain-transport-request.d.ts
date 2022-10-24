import * as flatbuffers from 'flatbuffers';
import { PlainTransportOptions, PlainTransportOptionsT } from '../../fbs/plain-transport/plain-transport-options';
export declare class CreatePlainTransportRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CreatePlainTransportRequest;
    static getRootAsCreatePlainTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CreatePlainTransportRequest): CreatePlainTransportRequest;
    static getSizePrefixedRootAsCreatePlainTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CreatePlainTransportRequest): CreatePlainTransportRequest;
    transportId(): string | null;
    transportId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    options(obj?: PlainTransportOptions): PlainTransportOptions | null;
    static startCreatePlainTransportRequest(builder: flatbuffers.Builder): void;
    static addTransportId(builder: flatbuffers.Builder, transportIdOffset: flatbuffers.Offset): void;
    static addOptions(builder: flatbuffers.Builder, optionsOffset: flatbuffers.Offset): void;
    static endCreatePlainTransportRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): CreatePlainTransportRequestT;
    unpackTo(_o: CreatePlainTransportRequestT): void;
}
export declare class CreatePlainTransportRequestT {
    transportId: string | Uint8Array | null;
    options: PlainTransportOptionsT | null;
    constructor(transportId?: string | Uint8Array | null, options?: PlainTransportOptionsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=create-plain-transport-request.d.ts.map