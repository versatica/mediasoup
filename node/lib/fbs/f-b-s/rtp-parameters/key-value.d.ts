import * as flatbuffers from 'flatbuffers';
import { Value } from '../../f-b-s/rtp-parameters/value';
export declare class KeyValue {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): KeyValue;
    static getRootAsKeyValue(bb: flatbuffers.ByteBuffer, obj?: KeyValue): KeyValue;
    static getSizePrefixedRootAsKeyValue(bb: flatbuffers.ByteBuffer, obj?: KeyValue): KeyValue;
    name(): string | null;
    name(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    valueType(): Value;
    value<T extends flatbuffers.Table>(obj: any): any | null;
    static startKeyValue(builder: flatbuffers.Builder): void;
    static addName(builder: flatbuffers.Builder, nameOffset: flatbuffers.Offset): void;
    static addValueType(builder: flatbuffers.Builder, valueType: Value): void;
    static addValue(builder: flatbuffers.Builder, valueOffset: flatbuffers.Offset): void;
    static endKeyValue(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createKeyValue(builder: flatbuffers.Builder, nameOffset: flatbuffers.Offset, valueType: Value, valueOffset: flatbuffers.Offset): flatbuffers.Offset;
}
//# sourceMappingURL=key-value.d.ts.map