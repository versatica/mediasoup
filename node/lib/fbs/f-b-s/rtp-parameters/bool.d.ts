import * as flatbuffers from 'flatbuffers';
export declare class Bool {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Bool;
    static getRootAsBool(bb: flatbuffers.ByteBuffer, obj?: Bool): Bool;
    static getSizePrefixedRootAsBool(bb: flatbuffers.ByteBuffer, obj?: Bool): Bool;
    value(): boolean;
    static startBool(builder: flatbuffers.Builder): void;
    static addValue(builder: flatbuffers.Builder, value: boolean): void;
    static endBool(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createBool(builder: flatbuffers.Builder, value: boolean): flatbuffers.Offset;
}
//# sourceMappingURL=bool.d.ts.map