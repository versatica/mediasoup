import * as flatbuffers from 'flatbuffers';
import { DumpResponse, DumpResponseT } from '../../fbs/consumer/dump-response';
import { Dump, DumpT } from '../../fbs/rtp-stream/dump';
export declare class SimpleConsumerDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SimpleConsumerDump;
    static getRootAsSimpleConsumerDump(bb: flatbuffers.ByteBuffer, obj?: SimpleConsumerDump): SimpleConsumerDump;
    static getSizePrefixedRootAsSimpleConsumerDump(bb: flatbuffers.ByteBuffer, obj?: SimpleConsumerDump): SimpleConsumerDump;
    base(obj?: DumpResponse): DumpResponse | null;
    rtpStream(obj?: Dump): Dump | null;
    static startSimpleConsumerDump(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addRtpStream(builder: flatbuffers.Builder, rtpStreamOffset: flatbuffers.Offset): void;
    static endSimpleConsumerDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): SimpleConsumerDumpT;
    unpackTo(_o: SimpleConsumerDumpT): void;
}
export declare class SimpleConsumerDumpT {
    base: DumpResponseT | null;
    rtpStream: DumpT | null;
    constructor(base?: DumpResponseT | null, rtpStream?: DumpT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=simple-consumer-dump.d.ts.map