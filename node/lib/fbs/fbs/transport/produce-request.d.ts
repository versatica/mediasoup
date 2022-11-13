import * as flatbuffers from 'flatbuffers';
import { MediaKind } from '../../fbs/rtp-parameters/media-kind';
import { RtpMapping, RtpMappingT } from '../../fbs/rtp-parameters/rtp-mapping';
import { RtpParameters, RtpParametersT } from '../../fbs/rtp-parameters/rtp-parameters';
export declare class ProduceRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ProduceRequest;
    static getRootAsProduceRequest(bb: flatbuffers.ByteBuffer, obj?: ProduceRequest): ProduceRequest;
    static getSizePrefixedRootAsProduceRequest(bb: flatbuffers.ByteBuffer, obj?: ProduceRequest): ProduceRequest;
    producerId(): string | null;
    producerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    kind(): MediaKind;
    rtpParameters(obj?: RtpParameters): RtpParameters | null;
    rtpMapping(obj?: RtpMapping): RtpMapping | null;
    keyFrameRequestDelay(): number;
    paused(): boolean;
    static startProduceRequest(builder: flatbuffers.Builder): void;
    static addProducerId(builder: flatbuffers.Builder, producerIdOffset: flatbuffers.Offset): void;
    static addKind(builder: flatbuffers.Builder, kind: MediaKind): void;
    static addRtpParameters(builder: flatbuffers.Builder, rtpParametersOffset: flatbuffers.Offset): void;
    static addRtpMapping(builder: flatbuffers.Builder, rtpMappingOffset: flatbuffers.Offset): void;
    static addKeyFrameRequestDelay(builder: flatbuffers.Builder, keyFrameRequestDelay: number): void;
    static addPaused(builder: flatbuffers.Builder, paused: boolean): void;
    static endProduceRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): ProduceRequestT;
    unpackTo(_o: ProduceRequestT): void;
}
export declare class ProduceRequestT {
    producerId: string | Uint8Array | null;
    kind: MediaKind;
    rtpParameters: RtpParametersT | null;
    rtpMapping: RtpMappingT | null;
    keyFrameRequestDelay: number;
    paused: boolean;
    constructor(producerId?: string | Uint8Array | null, kind?: MediaKind, rtpParameters?: RtpParametersT | null, rtpMapping?: RtpMappingT | null, keyFrameRequestDelay?: number, paused?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=produce-request.d.ts.map