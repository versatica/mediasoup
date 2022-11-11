import * as flatbuffers from 'flatbuffers';
export declare class CloseProducerRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CloseProducerRequest;
    static getRootAsCloseProducerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseProducerRequest): CloseProducerRequest;
    static getSizePrefixedRootAsCloseProducerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseProducerRequest): CloseProducerRequest;
    producerId(): string | null;
    producerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCloseProducerRequest(builder: flatbuffers.Builder): void;
    static addProducerId(builder: flatbuffers.Builder, producerIdOffset: flatbuffers.Offset): void;
    static endCloseProducerRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCloseProducerRequest(builder: flatbuffers.Builder, producerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CloseProducerRequestT;
    unpackTo(_o: CloseProducerRequestT): void;
}
export declare class CloseProducerRequestT {
    producerId: string | Uint8Array | null;
    constructor(producerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=close-producer-request.d.ts.map