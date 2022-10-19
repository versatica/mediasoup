import * as flatbuffers from 'flatbuffers';
export declare class ConsumerScore {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumerScore;
    static getRootAsConsumerScore(bb: flatbuffers.ByteBuffer, obj?: ConsumerScore): ConsumerScore;
    static getSizePrefixedRootAsConsumerScore(bb: flatbuffers.ByteBuffer, obj?: ConsumerScore): ConsumerScore;
    score(): number;
    producerScore(): number;
    producerScores(index: number): number | null;
    producerScoresLength(): number;
    producerScoresArray(): Uint8Array | null;
    static startConsumerScore(builder: flatbuffers.Builder): void;
    static addScore(builder: flatbuffers.Builder, score: number): void;
    static addProducerScore(builder: flatbuffers.Builder, producerScore: number): void;
    static addProducerScores(builder: flatbuffers.Builder, producerScoresOffset: flatbuffers.Offset): void;
    static createProducerScoresVector(builder: flatbuffers.Builder, data: number[] | Uint8Array): flatbuffers.Offset;
    static startProducerScoresVector(builder: flatbuffers.Builder, numElems: number): void;
    static endConsumerScore(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createConsumerScore(builder: flatbuffers.Builder, score: number, producerScore: number, producerScoresOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): ConsumerScoreT;
    unpackTo(_o: ConsumerScoreT): void;
}
export declare class ConsumerScoreT {
    score: number;
    producerScore: number;
    producerScores: (number)[];
    constructor(score?: number, producerScore?: number, producerScores?: (number)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=consumer-score.d.ts.map