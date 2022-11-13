import * as flatbuffers from 'flatbuffers';
export declare class AddProducerRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): AddProducerRequest;
    static getRootAsAddProducerRequest(bb: flatbuffers.ByteBuffer, obj?: AddProducerRequest): AddProducerRequest;
    static getSizePrefixedRootAsAddProducerRequest(bb: flatbuffers.ByteBuffer, obj?: AddProducerRequest): AddProducerRequest;
    producerId(): string | null;
    producerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startAddProducerRequest(builder: flatbuffers.Builder): void;
    static addProducerId(builder: flatbuffers.Builder, producerIdOffset: flatbuffers.Offset): void;
    static endAddProducerRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createAddProducerRequest(builder: flatbuffers.Builder, producerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): AddProducerRequestT;
    unpackTo(_o: AddProducerRequestT): void;
}
export declare class AddProducerRequestT {
    producerId: string | Uint8Array | null;
    constructor(producerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=add-producer-request.d.ts.map