import * as flatbuffers from 'flatbuffers';
import { DumpResponse, DumpResponseT } from '../../fbs/transport/dump-response';
export declare class DirectTransportDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DirectTransportDump;
    static getRootAsDirectTransportDump(bb: flatbuffers.ByteBuffer, obj?: DirectTransportDump): DirectTransportDump;
    static getSizePrefixedRootAsDirectTransportDump(bb: flatbuffers.ByteBuffer, obj?: DirectTransportDump): DirectTransportDump;
    base(obj?: DumpResponse): DumpResponse | null;
    static startDirectTransportDump(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static endDirectTransportDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDirectTransportDump(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): DirectTransportDumpT;
    unpackTo(_o: DirectTransportDumpT): void;
}
export declare class DirectTransportDumpT {
    base: DumpResponseT | null;
    constructor(base?: DumpResponseT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=direct-transport-dump.d.ts.map