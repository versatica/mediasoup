import * as flatbuffers from 'flatbuffers';
import { ConsumerLayers, ConsumerLayersT } from '../../fbs/consumer/consumer-layers';
export declare class SetPreferredLayersResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SetPreferredLayersResponse;
    static getRootAsSetPreferredLayersResponse(bb: flatbuffers.ByteBuffer, obj?: SetPreferredLayersResponse): SetPreferredLayersResponse;
    static getSizePrefixedRootAsSetPreferredLayersResponse(bb: flatbuffers.ByteBuffer, obj?: SetPreferredLayersResponse): SetPreferredLayersResponse;
    preferredLayers(obj?: ConsumerLayers): ConsumerLayers | null;
    static startSetPreferredLayersResponse(builder: flatbuffers.Builder): void;
    static addPreferredLayers(builder: flatbuffers.Builder, preferredLayersOffset: flatbuffers.Offset): void;
    static endSetPreferredLayersResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSetPreferredLayersResponse(builder: flatbuffers.Builder, preferredLayersOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): SetPreferredLayersResponseT;
    unpackTo(_o: SetPreferredLayersResponseT): void;
}
export declare class SetPreferredLayersResponseT {
    preferredLayers: ConsumerLayersT | null;
    constructor(preferredLayers?: ConsumerLayersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=set-preferred-layers-response.d.ts.map