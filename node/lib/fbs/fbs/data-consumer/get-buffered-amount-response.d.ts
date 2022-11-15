import * as flatbuffers from 'flatbuffers';
export declare class GetBufferedAmountResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): GetBufferedAmountResponse;
    static getRootAsGetBufferedAmountResponse(bb: flatbuffers.ByteBuffer, obj?: GetBufferedAmountResponse): GetBufferedAmountResponse;
    static getSizePrefixedRootAsGetBufferedAmountResponse(bb: flatbuffers.ByteBuffer, obj?: GetBufferedAmountResponse): GetBufferedAmountResponse;
    bufferedAmount(): number;
    static startGetBufferedAmountResponse(builder: flatbuffers.Builder): void;
    static addBufferedAmount(builder: flatbuffers.Builder, bufferedAmount: number): void;
    static endGetBufferedAmountResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createGetBufferedAmountResponse(builder: flatbuffers.Builder, bufferedAmount: number): flatbuffers.Offset;
    unpack(): GetBufferedAmountResponseT;
    unpackTo(_o: GetBufferedAmountResponseT): void;
}
export declare class GetBufferedAmountResponseT {
    bufferedAmount: number;
    constructor(bufferedAmount?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=get-buffered-amount-response.d.ts.map