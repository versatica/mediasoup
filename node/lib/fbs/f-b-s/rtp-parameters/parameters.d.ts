import * as flatbuffers from 'flatbuffers';
import { Value } from '../../f-b-s/rtp-parameters/value';
export declare class Parameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Parameters;
    static getRootAsParameters(bb: flatbuffers.ByteBuffer, obj?: Parameters): Parameters;
    static getSizePrefixedRootAsParameters(bb: flatbuffers.ByteBuffer, obj?: Parameters): Parameters;
    name(): string | null;
    name(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    valueType(): Value;
    value<T extends flatbuffers.Table>(obj: any): any | null;
    static startParameters(builder: flatbuffers.Builder): void;
    static addName(builder: flatbuffers.Builder, nameOffset: flatbuffers.Offset): void;
    static addValueType(builder: flatbuffers.Builder, valueType: Value): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createParameters(builder: flatbuffers.Builder, nameOffset: flatbuffers.Offset, valueType: Value, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
}
//# sourceMappingURL=parameters.d.ts.map