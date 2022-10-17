import * as flatbuffers from 'flatbuffers';
export declare class ConsumeResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumeResponse;
    static getRootAsConsumeResponse(bb: flatbuffers.ByteBuffer, obj?: ConsumeResponse): ConsumeResponse;
    static getSizePrefixedRootAsConsumeResponse(bb: flatbuffers.ByteBuffer, obj?: ConsumeResponse): ConsumeResponse;
    paused(): boolean;
    producerPaused(): boolean;
    score(): number;
    static startConsumeResponse(builder: flatbuffers.Builder): void;
    static addPaused(builder: flatbuffers.Builder, paused: boolean): void;
    static addProducerPaused(builder: flatbuffers.Builder, producerPaused: boolean): void;
    static addScore(builder: flatbuffers.Builder, score: number): void;
    static endConsumeResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createConsumeResponse(builder: flatbuffers.Builder, paused: boolean, producerPaused: boolean, score: number): flatbuffers.Offset;
}
//# sourceMappingURL=consume-response.d.ts.map