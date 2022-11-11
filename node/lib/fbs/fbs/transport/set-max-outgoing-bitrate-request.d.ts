import * as flatbuffers from 'flatbuffers';
export declare class SetMaxOutgoingBitrateRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SetMaxOutgoingBitrateRequest;
    static getRootAsSetMaxOutgoingBitrateRequest(bb: flatbuffers.ByteBuffer, obj?: SetMaxOutgoingBitrateRequest): SetMaxOutgoingBitrateRequest;
    static getSizePrefixedRootAsSetMaxOutgoingBitrateRequest(bb: flatbuffers.ByteBuffer, obj?: SetMaxOutgoingBitrateRequest): SetMaxOutgoingBitrateRequest;
    maxOutgoingBitrate(): number;
    static startSetMaxOutgoingBitrateRequest(builder: flatbuffers.Builder): void;
    static addMaxOutgoingBitrate(builder: flatbuffers.Builder, maxOutgoingBitrate: number): void;
    static endSetMaxOutgoingBitrateRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSetMaxOutgoingBitrateRequest(builder: flatbuffers.Builder, maxOutgoingBitrate: number): flatbuffers.Offset;
    unpack(): SetMaxOutgoingBitrateRequestT;
    unpackTo(_o: SetMaxOutgoingBitrateRequestT): void;
}
export declare class SetMaxOutgoingBitrateRequestT {
    maxOutgoingBitrate: number;
    constructor(maxOutgoingBitrate?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=set-max-outgoing-bitrate-request.d.ts.map