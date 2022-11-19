import * as flatbuffers from 'flatbuffers';
import { Params, ParamsT } from '../../fbs/rtx-stream/params';
export declare class RtxDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RtxDump;
    static getRootAsRtxDump(bb: flatbuffers.ByteBuffer, obj?: RtxDump): RtxDump;
    static getSizePrefixedRootAsRtxDump(bb: flatbuffers.ByteBuffer, obj?: RtxDump): RtxDump;
    params(obj?: Params): Params | null;
    static startRtxDump(builder: flatbuffers.Builder): void;
    static addParams(builder: flatbuffers.Builder, paramsOffset: flatbuffers.Offset): void;
    static endRtxDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createRtxDump(builder: flatbuffers.Builder, paramsOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): RtxDumpT;
    unpackTo(_o: RtxDumpT): void;
}
export declare class RtxDumpT {
    params: ParamsT | null;
    constructor(params?: ParamsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=rtx-dump.d.ts.map