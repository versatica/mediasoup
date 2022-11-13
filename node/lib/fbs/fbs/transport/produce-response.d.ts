import * as flatbuffers from 'flatbuffers';
import { Type } from '../../fbs/rtp-parameters/type';
export declare class ProduceResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ProduceResponse;
    static getRootAsProduceResponse(bb: flatbuffers.ByteBuffer, obj?: ProduceResponse): ProduceResponse;
    static getSizePrefixedRootAsProduceResponse(bb: flatbuffers.ByteBuffer, obj?: ProduceResponse): ProduceResponse;
    type(): Type;
    static startProduceResponse(builder: flatbuffers.Builder): void;
    static addType(builder: flatbuffers.Builder, type: Type): void;
    static endProduceResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createProduceResponse(builder: flatbuffers.Builder, type: Type): flatbuffers.Offset;
    unpack(): ProduceResponseT;
    unpackTo(_o: ProduceResponseT): void;
}
export declare class ProduceResponseT {
    type: Type;
    constructor(type?: Type);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=produce-response.d.ts.map