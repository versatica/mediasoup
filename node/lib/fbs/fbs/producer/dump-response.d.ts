import * as flatbuffers from 'flatbuffers';
import { MediaKind } from '../../fbs/rtp-parameters/media-kind';
import { RtpMapping, RtpMappingT } from '../../fbs/rtp-parameters/rtp-mapping';
import { RtpParameters, RtpParametersT } from '../../fbs/rtp-parameters/rtp-parameters';
import { Dump, DumpT } from '../../fbs/rtp-stream/dump';
export declare class DumpResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DumpResponse;
    static getRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    static getSizePrefixedRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    id(): string | null;
    id(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    kind(): MediaKind;
    type(): string | null;
    type(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    rtpParameters(obj?: RtpParameters): RtpParameters | null;
    rtpMapping(obj?: RtpMapping): RtpMapping | null;
    rtpStreams(index: number, obj?: Dump): Dump | null;
    rtpStreamsLength(): number;
    traceEventTypes(index: number): string;
    traceEventTypes(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    traceEventTypesLength(): number;
    paused(): boolean;
    static startDumpResponse(builder: flatbuffers.Builder): void;
    static addId(builder: flatbuffers.Builder, idOffset: flatbuffers.Offset): void;
    static addKind(builder: flatbuffers.Builder, kind: MediaKind): void;
    static addType(builder: flatbuffers.Builder, typeOffset: flatbuffers.Offset): void;
    static addRtpParameters(builder: flatbuffers.Builder, rtpParametersOffset: flatbuffers.Offset): void;
    static addRtpMapping(builder: flatbuffers.Builder, rtpMappingOffset: flatbuffers.Offset): void;
    static addRtpStreams(builder: flatbuffers.Builder, rtpStreamsOffset: flatbuffers.Offset): void;
    static createRtpStreamsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRtpStreamsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addTraceEventTypes(builder: flatbuffers.Builder, traceEventTypesOffset: flatbuffers.Offset): void;
    static createTraceEventTypesVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startTraceEventTypesVector(builder: flatbuffers.Builder, numElems: number): void;
    static addPaused(builder: flatbuffers.Builder, paused: boolean): void;
    static endDumpResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): DumpResponseT;
    unpackTo(_o: DumpResponseT): void;
}
export declare class DumpResponseT {
    id: string | Uint8Array | null;
    kind: MediaKind;
    type: string | Uint8Array | null;
    rtpParameters: RtpParametersT | null;
    rtpMapping: RtpMappingT | null;
    rtpStreams: (DumpT)[];
    traceEventTypes: (string)[];
    paused: boolean;
    constructor(id?: string | Uint8Array | null, kind?: MediaKind, type?: string | Uint8Array | null, rtpParameters?: RtpParametersT | null, rtpMapping?: RtpMappingT | null, rtpStreams?: (DumpT)[], traceEventTypes?: (string)[], paused?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump-response.d.ts.map