import * as flatbuffers from 'flatbuffers';
export declare class Rtx {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Rtx;
    ssrc(): number;
    static sizeOf(): number;
    static createRtx(builder: flatbuffers.Builder, ssrc: number): flatbuffers.Offset;
}
//# sourceMappingURL=rtx.d.ts.map