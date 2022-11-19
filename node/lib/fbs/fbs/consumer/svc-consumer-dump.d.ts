import * as flatbuffers from 'flatbuffers';
import { DumpResponse, DumpResponseT } from '../../fbs/consumer/dump-response';
import { Dump, DumpT } from '../../fbs/rtp-stream/dump';
export declare class SvcConsumerDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SvcConsumerDump;
    static getRootAsSvcConsumerDump(bb: flatbuffers.ByteBuffer, obj?: SvcConsumerDump): SvcConsumerDump;
    static getSizePrefixedRootAsSvcConsumerDump(bb: flatbuffers.ByteBuffer, obj?: SvcConsumerDump): SvcConsumerDump;
    base(obj?: DumpResponse): DumpResponse | null;
    rtpStream(obj?: Dump): Dump | null;
    preferredSpatialLayer(): number;
    targetSpatialLayer(): number;
    currentSpatialLayer(): number;
    preferredTemporalLayer(): number;
    targetTemporalLayer(): number;
    currentTemporalLayer(): number;
    static startSvcConsumerDump(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addRtpStream(builder: flatbuffers.Builder, rtpStreamOffset: flatbuffers.Offset): void;
    static addPreferredSpatialLayer(builder: flatbuffers.Builder, preferredSpatialLayer: number): void;
    static addTargetSpatialLayer(builder: flatbuffers.Builder, targetSpatialLayer: number): void;
    static addCurrentSpatialLayer(builder: flatbuffers.Builder, currentSpatialLayer: number): void;
    static addPreferredTemporalLayer(builder: flatbuffers.Builder, preferredTemporalLayer: number): void;
    static addTargetTemporalLayer(builder: flatbuffers.Builder, targetTemporalLayer: number): void;
    static addCurrentTemporalLayer(builder: flatbuffers.Builder, currentTemporalLayer: number): void;
    static endSvcConsumerDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): SvcConsumerDumpT;
    unpackTo(_o: SvcConsumerDumpT): void;
}
export declare class SvcConsumerDumpT {
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
//# sourceMappingURL=svc-consumer-dump.d.ts.map