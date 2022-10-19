import * as flatbuffers from 'flatbuffers';
export declare class ConsumerLayers {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumerLayers;
    static getRootAsConsumerLayers(bb: flatbuffers.ByteBuffer, obj?: ConsumerLayers): ConsumerLayers;
    static getSizePrefixedRootAsConsumerLayers(bb: flatbuffers.ByteBuffer, obj?: ConsumerLayers): ConsumerLayers;
    spatialLayer(): number;
    temporalLayer(): number;
    static startConsumerLayers(builder: flatbuffers.Builder): void;
    static addSpatialLayer(builder: flatbuffers.Builder, spatialLayer: number): void;
    static addTemporalLayer(builder: flatbuffers.Builder, temporalLayer: number): void;
    static endConsumerLayers(builder: flatbuffers.Builder): flatbuffers.Offset;
    static finishConsumerLayersBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static finishSizePrefixedConsumerLayersBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static createConsumerLayers(builder: flatbuffers.Builder, spatialLayer: number, temporalLayer: number): flatbuffers.Offset;
    unpack(): ConsumerLayersT;
    unpackTo(_o: ConsumerLayersT): void;
}
export declare class ConsumerLayersT {
    spatialLayer: number;
    temporalLayer: number;
    constructor(spatialLayer?: number, temporalLayer?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=consumer-layers.d.ts.map