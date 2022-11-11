import * as flatbuffers from 'flatbuffers';
export declare class CloseDataProducerRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CloseDataProducerRequest;
    static getRootAsCloseDataProducerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseDataProducerRequest): CloseDataProducerRequest;
    static getSizePrefixedRootAsCloseDataProducerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseDataProducerRequest): CloseDataProducerRequest;
    dataProducerId(): string | null;
    dataProducerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCloseDataProducerRequest(builder: flatbuffers.Builder): void;
    static addDataProducerId(builder: flatbuffers.Builder, dataProducerIdOffset: flatbuffers.Offset): void;
    static endCloseDataProducerRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCloseDataProducerRequest(builder: flatbuffers.Builder, dataProducerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CloseDataProducerRequestT;
    unpackTo(_o: CloseDataProducerRequestT): void;
}
export declare class CloseDataProducerRequestT {
    dataProducerId: string | Uint8Array | null;
    constructor(dataProducerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=close-data-producer-request.d.ts.map