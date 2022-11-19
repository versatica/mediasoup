import * as flatbuffers from 'flatbuffers';
import { Params, ParamsT } from '../../fbs/rtp-stream/params';
import { RtxDump, RtxDumpT } from '../../fbs/rtx-stream/rtx-dump';
export declare class Dump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Dump;
    static getRootAsDump(bb: flatbuffers.ByteBuffer, obj?: Dump): Dump;
    static getSizePrefixedRootAsDump(bb: flatbuffers.ByteBuffer, obj?: Dump): Dump;
    params(obj?: Params): Params | null;
    score(): number;
    rtxStream(obj?: RtxDump): RtxDump | null;
    static startDump(builder: flatbuffers.Builder): void;
    static addParams(builder: flatbuffers.Builder, paramsOffset: flatbuffers.Offset): void;
    static addScore(builder: flatbuffers.Builder, score: number): void;
    static addRtxStream(builder: flatbuffers.Builder, rtxStreamOffset: flatbuffers.Offset): void;
    static endDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): DumpT;
    unpackTo(_o: DumpT): void;
}
export declare class DumpT {
    params: ParamsT | null;
    score: number;
    rtxStream: RtxDumpT | null;
    constructor(params?: ParamsT | null, score?: number, rtxStream?: RtxDumpT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump.d.ts.map