import * as flatbuffers from 'flatbuffers';
import { DumpResponse, DumpResponseT } from '../../fbs/consumer/dump-response';
import { Dump, DumpT } from '../../fbs/rtp-stream/dump';
export declare class SimulcastConsumerDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SimulcastConsumerDump;
    static getRootAsSimulcastConsumerDump(bb: flatbuffers.ByteBuffer, obj?: SimulcastConsumerDump): SimulcastConsumerDump;
    static getSizePrefixedRootAsSimulcastConsumerDump(bb: flatbuffers.ByteBuffer, obj?: SimulcastConsumerDump): SimulcastConsumerDump;
    base(obj?: DumpResponse): DumpResponse | null;
    rtpStream(obj?: Dump): Dump | null;
    preferredSpatialLayer(): number;
    targetSpatialLayer(): number;
    currentSpatialLayer(): number;
    preferredTemporalLayer(): number;
    targetTemporalLayer(): number;
    currentTemporalLayer(): number;
    static startSimulcastConsumerDump(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addRtpStream(builder: flatbuffers.Builder, rtpStreamOffset: flatbuffers.Offset): void;
    static addPreferredSpatialLayer(builder: flatbuffers.Builder, preferredSpatialLayer: number): void;
    static addTargetSpatialLayer(builder: flatbuffers.Builder, targetSpatialLayer: number): void;
    static addCurrentSpatialLayer(builder: flatbuffers.Builder, currentSpatialLayer: number): void;
    static addPreferredTemporalLayer(builder: flatbuffers.Builder, preferredTemporalLayer: number): void;
    static addTargetTemporalLayer(builder: flatbuffers.Builder, targetTemporalLayer: number): void;
    static addCurrentTemporalLayer(builder: flatbuffers.Builder, currentTemporalLayer: number): void;
    static endSimulcastConsumerDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): SimulcastConsumerDumpT;
    unpackTo(_o: SimulcastConsumerDumpT): void;
}
export declare class SimulcastConsumerDumpT {
    base: DumpResponseT | null;
    rtpStream: DumpT | null;
    preferredSpatialLayer: number;
    targetSpatialLayer: number;
    currentSpatialLayer: number;
    preferredTemporalLayer: number;
    targetTemporalLayer: number;
    currentTemporalLayer: number;
    constructor(base?: DumpResponseT | null, rtpStream?: DumpT | null, preferredSpatialLayer?: number, targetSpatialLayer?: number, currentSpatialLayer?: number, preferredTemporalLayer?: number, targetTemporalLayer?: number, currentTemporalLayer?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=simulcast-consumer-dump.d.ts.map