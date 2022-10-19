import * as flatbuffers from 'flatbuffers';
import { ConsumerLayers, ConsumerLayersT } from '../../fbs/consumer/consumer-layers';
import { ConsumerScore, ConsumerScoreT } from '../../fbs/consumer/consumer-score';
export declare class ConsumeResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumeResponse;
    static getRootAsConsumeResponse(bb: flatbuffers.ByteBuffer, obj?: ConsumeResponse): ConsumeResponse;
    static getSizePrefixedRootAsConsumeResponse(bb: flatbuffers.ByteBuffer, obj?: ConsumeResponse): ConsumeResponse;
    paused(): boolean;
    producerPaused(): boolean;
    score(obj?: ConsumerScore): ConsumerScore | null;
    preferredLayers(obj?: ConsumerLayers): ConsumerLayers | null;
    static startConsumeResponse(builder: flatbuffers.Builder): void;
    static addPaused(builder: flatbuffers.Builder, paused: boolean): void;
    static addProducerPaused(builder: flatbuffers.Builder, producerPaused: boolean): void;
    static addScore(builder: flatbuffers.Builder, scoreOffset: flatbuffers.Offset): void;
    static addPreferredLayers(builder: flatbuffers.Builder, preferredLayersOffset: flatbuffers.Offset): void;
    static endConsumeResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): ConsumeResponseT;
    unpackTo(_o: ConsumeResponseT): void;
}
export declare class ConsumeResponseT {
    paused: boolean;
    producerPaused: boolean;
    score: ConsumerScoreT | null;
    preferredLayers: ConsumerLayersT | null;
    constructor(paused?: boolean, producerPaused?: boolean, score?: ConsumerScoreT | null, preferredLayers?: ConsumerLayersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=consume-response.d.ts.map