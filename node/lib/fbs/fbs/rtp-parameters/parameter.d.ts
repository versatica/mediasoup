import * as flatbuffers from 'flatbuffers';
import { BooleanT } from '../../fbs/rtp-parameters/boolean';
import { DoubleT } from '../../fbs/rtp-parameters/double';
import { IntegerT } from '../../fbs/rtp-parameters/integer';
import { IntegerArrayT } from '../../fbs/rtp-parameters/integer-array';
import { StringT } from '../../fbs/rtp-parameters/string';
import { Value } from '../../fbs/rtp-parameters/value';
export declare class Parameter {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Parameter;
    static getRootAsParameter(bb: flatbuffers.ByteBuffer, obj?: Parameter): Parameter;
    static getSizePrefixedRootAsParameter(bb: flatbuffers.ByteBuffer, obj?: Parameter): Parameter;
    name(): string | null;
    name(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    valueType(): Value;
    value<T extends flatbuffers.Table>(obj: any): any | null;
    static startParameter(builder: flatbuffers.Builder): void;
    static addName(builder: flatbuffers.Builder, nameOffset: flatbuffers.Offset): void;
    static addValueType(builder: flatbuffers.Builder, valueType: Value): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endParameter(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createParameter(builder: flatbuffers.Builder, nameOffset: flatbuffers.Offset, valueType: Value, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): ParameterT;
    unpackTo(_o: ParameterT): void;
}
export declare class ParameterT {
    name: string | Uint8Array | null;
    valueType: Value;
    value: BooleanT | DoubleT | IntegerArrayT | IntegerT | StringT | null;
    constructor(name?: string | Uint8Array | null, valueType?: Value, value?: BooleanT | DoubleT | IntegerArrayT | IntegerT | StringT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=parameter.d.ts.map