import * as flatbuffers from 'flatbuffers';
export declare class ConsumerLayers {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumerLayers;
    spatialLayer(): number;
    temporalLayer(): number;
    static sizeOf(): number;
    static createConsumerLayers(builder: flatbuffers.Builder, spatial_layer: number, temporal_layer: number): flatbuffers.Offset;
}
//# sourceMappingURL=consumer-layers.d.ts.map