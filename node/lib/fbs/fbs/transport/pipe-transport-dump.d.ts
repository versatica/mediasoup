import * as flatbuffers from 'flatbuffers';
import { DumpResponse, DumpResponseT } from '../../fbs/transport/dump-response';
import { SrtpParameters, SrtpParametersT } from '../../fbs/transport/srtp-parameters';
import { Tuple, TupleT } from '../../fbs/transport/tuple';
export declare class PipeTransportDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): PipeTransportDump;
    static getRootAsPipeTransportDump(bb: flatbuffers.ByteBuffer, obj?: PipeTransportDump): PipeTransportDump;
    static getSizePrefixedRootAsPipeTransportDump(bb: flatbuffers.ByteBuffer, obj?: PipeTransportDump): PipeTransportDump;
    base(obj?: DumpResponse): DumpResponse | null;
    tuple(obj?: Tuple): Tuple | null;
    rtx(): boolean;
    srtpParameters(obj?: SrtpParameters): SrtpParameters | null;
    static startPipeTransportDump(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addTuple(builder: flatbuffers.Builder, tupleOffset: flatbuffers.Offset): void;
    static addRtx(builder: flatbuffers.Builder, rtx: boolean): void;
    static addSrtpParameters(builder: flatbuffers.Builder, srtpParametersOffset: flatbuffers.Offset): void;
    static endPipeTransportDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): PipeTransportDumpT;
    unpackTo(_o: PipeTransportDumpT): void;
}
export declare class PipeTransportDumpT {
    base: DumpResponseT | null;
    tuple: TupleT | null;
    rtx: boolean;
    srtpParameters: SrtpParametersT | null;
    constructor(base?: DumpResponseT | null, tuple?: TupleT | null, rtx?: boolean, srtpParameters?: SrtpParametersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=pipe-transport-dump.d.ts.map