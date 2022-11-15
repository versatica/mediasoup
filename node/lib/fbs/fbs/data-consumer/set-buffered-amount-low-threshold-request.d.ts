import * as flatbuffers from 'flatbuffers';
export declare class SetBufferedAmountLowThresholdRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SetBufferedAmountLowThresholdRequest;
    static getRootAsSetBufferedAmountLowThresholdRequest(bb: flatbuffers.ByteBuffer, obj?: SetBufferedAmountLowThresholdRequest): SetBufferedAmountLowThresholdRequest;
    static getSizePrefixedRootAsSetBufferedAmountLowThresholdRequest(bb: flatbuffers.ByteBuffer, obj?: SetBufferedAmountLowThresholdRequest): SetBufferedAmountLowThresholdRequest;
    threshold(): number;
    static startSetBufferedAmountLowThresholdRequest(builder: flatbuffers.Builder): void;
    static addThreshold(builder: flatbuffers.Builder, threshold: number): void;
    static endSetBufferedAmountLowThresholdRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSetBufferedAmountLowThresholdRequest(builder: flatbuffers.Builder, threshold: number): flatbuffers.Offset;
    unpack(): SetBufferedAmountLowThresholdRequestT;
    unpackTo(_o: SetBufferedAmountLowThresholdRequestT): void;
}
export declare class SetBufferedAmountLowThresholdRequestT {
    threshold: number;
    constructor(threshold?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=set-buffered-amount-low-threshold-request.d.ts.map