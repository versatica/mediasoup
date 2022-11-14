import * as flatbuffers from 'flatbuffers';
import { OptionalUint16, OptionalUint16T } from '../../fbs/common/optional-uint16';
export declare class ConsumerLayers {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumerLayers;
    static getRootAsConsumerLayers(bb: flatbuffers.ByteBuffer, obj?: ConsumerLayers): ConsumerLayers;
    static getSizePrefixedRootAsConsumerLayers(bb: flatbuffers.ByteBuffer, obj?: ConsumerLayers): ConsumerLayers;
    spatialLayer(): number;
    temporalLayer(obj?: OptionalUint16): OptionalUint16 | null;
    static startConsumerLayers(builder: flatbuffers.Builder): void;
    static addSpatialLayer(builder: flatbuffers.Builder, spatialLayer: number): void;
    static addTemporalLayer(builder: flatbuffers.Builder, temporalLayerOffset: flatbuffers.Offset): void;
    static endConsumerLayers(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): ConsumerLayersT;
    unpackTo(_o: ConsumerLayersT): void;
}
export declare class ConsumerLayersT {
    spatialLayer: number;
    temporalLayer: OptionalUint16T | null;
    constructor(spatialLayer?: number, temporalLayer?: OptionalUint16T | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=consumer-layers.d.ts.map