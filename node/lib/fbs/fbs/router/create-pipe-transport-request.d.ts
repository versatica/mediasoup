import * as flatbuffers from 'flatbuffers';
import { PipeTransportOptions, PipeTransportOptionsT } from '../../fbs/pipe-transport/pipe-transport-options';
export declare class CreatePipeTransportRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CreatePipeTransportRequest;
    static getRootAsCreatePipeTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CreatePipeTransportRequest): CreatePipeTransportRequest;
    static getSizePrefixedRootAsCreatePipeTransportRequest(bb: flatbuffers.ByteBuffer, obj?: CreatePipeTransportRequest): CreatePipeTransportRequest;
    transportId(): string | null;
    transportId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    options(obj?: PipeTransportOptions): PipeTransportOptions | null;
    static startCreatePipeTransportRequest(builder: flatbuffers.Builder): void;
    static addTransportId(builder: flatbuffers.Builder, transportIdOffset: flatbuffers.Offset): void;
    static addOptions(builder: flatbuffers.Builder, optionsOffset: flatbuffers.Offset): void;
    static endCreatePipeTransportRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): CreatePipeTransportRequestT;
    unpackTo(_o: CreatePipeTransportRequestT): void;
}
export declare class CreatePipeTransportRequestT {
    transportId: string | Uint8Array | null;
    options: PipeTransportOptionsT | null;
    constructor(transportId?: string | Uint8Array | null, options?: PipeTransportOptionsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=create-pipe-transport-request.d.ts.map