import * as flatbuffers from 'flatbuffers';
export declare class CloseConsumerRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CloseConsumerRequest;
    static getRootAsCloseConsumerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseConsumerRequest): CloseConsumerRequest;
    static getSizePrefixedRootAsCloseConsumerRequest(bb: flatbuffers.ByteBuffer, obj?: CloseConsumerRequest): CloseConsumerRequest;
    consumerId(): string | null;
    consumerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startCloseConsumerRequest(builder: flatbuffers.Builder): void;
    static addConsumerId(builder: flatbuffers.Builder, consumerIdOffset: flatbuffers.Offset): void;
    static endCloseConsumerRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createCloseConsumerRequest(builder: flatbuffers.Builder, consumerIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): CloseConsumerRequestT;
    unpackTo(_o: CloseConsumerRequestT): void;
}
export declare class CloseConsumerRequestT {
    consumerId: string | Uint8Array | null;
    constructor(consumerId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=close-consumer-request.d.ts.map