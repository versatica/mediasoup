import * as flatbuffers from 'flatbuffers';
export declare class fake {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): fake;
    static getRootAsfake(bb: flatbuffers.ByteBuffer, obj?: fake): fake;
    static getSizePrefixedRootAsfake(bb: flatbuffers.ByteBuffer, obj?: fake): fake;
    foo(): number;
    static startfake(builder: flatbuffers.Builder): void;
    static addFoo(builder: flatbuffers.Builder, foo: number): void;
    static endfake(builder: flatbuffers.Builder): flatbuffers.Offset;
    static finishfakeBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static finishSizePrefixedfakeBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static createfake(builder: flatbuffers.Builder, foo: number): flatbuffers.Offset;
}
//# sourceMappingURL=fake.d.ts.map