import * as flatbuffers from 'flatbuffers';
export declare class SetMaxIncomingBitrateRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SetMaxIncomingBitrateRequest;
    static getRootAsSetMaxIncomingBitrateRequest(bb: flatbuffers.ByteBuffer, obj?: SetMaxIncomingBitrateRequest): SetMaxIncomingBitrateRequest;
    static getSizePrefixedRootAsSetMaxIncomingBitrateRequest(bb: flatbuffers.ByteBuffer, obj?: SetMaxIncomingBitrateRequest): SetMaxIncomingBitrateRequest;
    maxIncomingBitrate(): number;
    static startSetMaxIncomingBitrateRequest(builder: flatbuffers.Builder): void;
    static addMaxIncomingBitrate(builder: flatbuffers.Builder, maxIncomingBitrate: number): void;
    static endSetMaxIncomingBitrateRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSetMaxIncomingBitrateRequest(builder: flatbuffers.Builder, maxIncomingBitrate: number): flatbuffers.Offset;
    unpack(): SetMaxIncomingBitrateRequestT;
    unpackTo(_o: SetMaxIncomingBitrateRequestT): void;
}
export declare class SetMaxIncomingBitrateRequestT {
    maxIncomingBitrate: number;
    constructor(maxIncomingBitrate?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=set-max-incoming-bitrate-request.d.ts.map