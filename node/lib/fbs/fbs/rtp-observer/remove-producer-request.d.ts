import * as flatbuffers from 'flatbuffers';
export declare class RemoveProducerRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RemoveProducerRequest;
    static getRootAsRemoveProducerRequest(bb: flatbuffers.ByteBuffer, obj?: RemoveProducerRequest): RemoveProducerRequest;
    static getSizePrefixedRootAsRemoveProducerRequest(bb: flatbuffers.ByteBuffer, obj?: RemoveProducerRequest): RemoveProducerRequest;
    producerId(): string | null;
    producerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startRemoveProducerRequest(builder: flatbuffers.Builder): void;
    static addProducerId(builder: flatbuffers.Builder, producerIdOffset: flatbuffers.Offset): void;
    static endRemoveProducerRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createRemoveProducerRequest(builder: flatbuffers.Builder, producerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): RemoveProducerRequestT;
    unpackTo(_o: RemoveProducerRequestT): void;
}
export declare class RemoveProducerRequestT {
    producerId: string | Uint8Array | null;
    constructor(producerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=remove-producer-request.d.ts.map