import * as flatbuffers from 'flatbuffers';
import { OptionalInt16, OptionalInt16T } from '../../fbs/common/optional-int16';
export declare class ConsumerLayers {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumerLayers;
    static getRootAsConsumerLayers(bb: flatbuffers.ByteBuffer, obj?: ConsumerLayers): ConsumerLayers;
    static getSizePrefixedRootAsConsumerLayers(bb: flatbuffers.ByteBuffer, obj?: ConsumerLayers): ConsumerLayers;
    spatialLayer(): number;
    temporalLayer(obj?: OptionalInt16): OptionalInt16 | null;
    static startConsumerLayers(builder: flatbuffers.Builder): void;
    static addSpatialLayer(builder: flatbuffers.Builder, spatialLayer: number): void;
    static addTemporalLayer(builder: flatbuffers.Builder, temporalLayerOffset: flatbuffers.Offset): void;
    static endConsumerLayers(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): ConsumerLayersT;
    unpackTo(_o: ConsumerLayersT): void;
}
export declare class ConsumerLayersT {
    spatialLayer: number;
    temporalLayer: OptionalInt16T | null;
    constructor(spatialLayer?: number, temporalLayer?: OptionalInt16T | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=consumer-layers.d.ts.map