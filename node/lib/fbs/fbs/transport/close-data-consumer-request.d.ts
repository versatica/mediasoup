import * as flatbuffers from 'flatbuffers';
export declare class CloseDataConsumerRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CloseDataConsumerRequest;
    static getRootAsCloseDataConsumerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseDataConsumerRequest): CloseDataConsumerRequest;
    static getSizePrefixedRootAsCloseDataConsumerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseDataConsumerRequest): CloseDataConsumerRequest;
    dataConsumerId(): string | null;
    dataConsumerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCloseDataConsumerRequest(builder: flatbuffers.Builder): void;
    static addDataConsumerId(builder: flatbuffers.Builder, dataConsumerIdOffset: flatbuffers.Offset): void;
    static endCloseDataConsumerRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCloseDataConsumerRequest(builder: flatbuffers.Builder, dataConsumerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CloseDataConsumerRequestT;
    unpackTo(_o: CloseDataConsumerRequestT): void;
}
export declare class CloseDataConsumerRequestT {
    dataConsumerId: string | Uint8Array | null;
    constructor(dataConsumerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=close-data-consumer-request.d.ts.map