import * as flatbuffers from 'flatbuffers';
import { DumpResponse, DumpResponseT } from '../../fbs/consumer/dump-response';
import { Dump, DumpT } from '../../fbs/rtp-stream/dump';
export declare class PipeConsumerDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): PipeConsumerDump;
    static getRootAsPipeConsumerDump(bb: flatbuffers.ByteBuffer, obj?: PipeConsumerDump): PipeConsumerDump;
    static getSizePrefixedRootAsPipeConsumerDump(bb: flatbuffers.ByteBuffer, obj?: PipeConsumerDump): PipeConsumerDump;
    base(obj?: DumpResponse): DumpResponse | null;
    rtpStream(index: number, obj?: Dump): Dump | null;
    rtpStreamLength(): number;
    static startPipeConsumerDump(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addRtpStream(builder: flatbuffers.Builder, rtpStreamOffset: flatbuffers.Offset): void;
    static createRtpStreamVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRtpStreamVector(builder: flatbuffers.Builder, numElems: number): void;
    static endPipeConsumerDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createPipeConsumerDump(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset, rtpStreamOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): PipeConsumerDumpT;
    unpackTo(_o: PipeConsumerDumpT): void;
}
export declare class PipeConsumerDumpT {
    base: DumpResponseT | null;
    rtpStream: (DumpT)[];
    constructor(base?: DumpResponseT | null, rtpStream?: (DumpT)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=pipe-consumer-dump.d.ts.map