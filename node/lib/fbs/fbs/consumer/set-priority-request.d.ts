import * as flatbuffers from 'flatbuffers';
export declare class SetPriorityRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SetPriorityRequest;
    static getRootAsSetPriorityRequest(bb: flatbuffers.ByteBuffer, obj?: SetPriorityRequest): SetPriorityRequest;
    static getSizePrefixedRootAsSetPriorityRequest(bb: flatbuffers.ByteBuffer, obj?: SetPriorityRequest): SetPriorityRequest;
    priority(): number;
    static startSetPriorityRequest(builder: flatbuffers.Builder): void;
    static addPriority(builder: flatbuffers.Builder, priority: number): void;
    static endSetPriorityRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSetPriorityRequest(builder: flatbuffers.Builder, priority: number): flatbuffers.Offset;
    unpack(): SetPriorityRequestT;
    unpackTo(_o: SetPriorityRequestT): void;
}
export declare class SetPriorityRequestT {
    priority: number;
    constructor(priority?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=set-priority-request.d.ts.map