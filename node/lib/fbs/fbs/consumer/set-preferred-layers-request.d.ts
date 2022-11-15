import * as flatbuffers from 'flatbuffers';
import { ConsumerLayers, ConsumerLayersT } from '../../fbs/consumer/consumer-layers';
export declare class SetPreferredLayersRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SetPreferredLayersRequest;
    static getRootAsSetPreferredLayersRequest(bb: flatbuffers.ByteBuffer, obj?: SetPreferredLayersRequest): SetPreferredLayersRequest;
    static getSizePrefixedRootAsSetPreferredLayersRequest(bb: flatbuffers.ByteBuffer, obj?: SetPreferredLayersRequest): SetPreferredLayersRequest;
    preferredLayers(obj?: ConsumerLayers): ConsumerLayers | null;
    static startSetPreferredLayersRequest(builder: flatbuffers.Builder): void;
    static addPreferredLayers(builder: flatbuffers.Builder, preferredLayersOffset: flatbuffers.Offset): void;
    static endSetPreferredLayersRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSetPreferredLayersRequest(builder: flatbuffers.Builder, preferredLayersOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): SetPreferredLayersRequestT;
    unpackTo(_o: SetPreferredLayersRequestT): void;
}
export declare class SetPreferredLayersRequestT {
    preferredLayers: ConsumerLayersT | null;
    constructor(preferredLayers?: ConsumerLayersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=set-preferred-layers-request.d.ts.map