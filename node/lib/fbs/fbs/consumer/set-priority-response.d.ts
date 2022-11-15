import * as flatbuffers from 'flatbuffers';
export declare class SetPriorityResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SetPriorityResponse;
    static getRootAsSetPriorityResponse(bb: flatbuffers.ByteBuffer, obj?: SetPriorityResponse): SetPriorityResponse;
    static getSizePrefixedRootAsSetPriorityResponse(bb: flatbuffers.ByteBuffer, obj?: SetPriorityResponse): SetPriorityResponse;
    priority(): number;
    static startSetPriorityResponse(builder: flatbuffers.Builder): void;
    static addPriority(builder: flatbuffers.Builder, priority: number): void;
    static endSetPriorityResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSetPriorityResponse(builder: flatbuffers.Builder, priority: number): flatbuffers.Offset;
    unpack(): SetPriorityResponseT;
    unpackTo(_o: SetPriorityResponseT): void;
}
export declare class SetPriorityResponseT {
    priority: number;
    constructor(priority?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=set-priority-response.d.ts.map