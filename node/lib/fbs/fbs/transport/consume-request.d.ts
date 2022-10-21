import * as flatbuffers from 'flatbuffers';
import { ConsumerLayers, ConsumerLayersT } from '../../fbs/consumer/consumer-layers';
import { MediaKind } from '../../fbs/rtp-parameters/media-kind';
import { RtpEncodingParameters, RtpEncodingParametersT } from '../../fbs/rtp-parameters/rtp-encoding-parameters';
import { RtpParameters, RtpParametersT } from '../../fbs/rtp-parameters/rtp-parameters';
import { Type } from '../../fbs/rtp-parameters/type';
export declare class ConsumeRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ConsumeRequest;
    static getRootAsConsumeRequest(bb: flatbuffers.ByteBuffer, obj?: ConsumeRequest): ConsumeRequest;
    static getSizePrefixedRootAsConsumeRequest(bb: flatbuffers.ByteBuffer, obj?: ConsumeRequest): ConsumeRequest;
    consumerId(): string | null;
    consumerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    producerId(): string | null;
    producerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    kind(): MediaKind;
    rtpParameters(obj?: RtpParameters): RtpParameters | null;
    type(): Type;
    consumableRtpEncodings(index: number, obj?: RtpEncodingParameters): RtpEncodingParameters | null;
    consumableRtpEncodingsLength(): number;
    paused(): boolean;
    preferredLayers(obj?: ConsumerLayers): ConsumerLayers | null;
    ignoreDtx(): boolean;
    static startConsumeRequest(builder: flatbuffers.Builder): void;
    static addConsumerId(builder: flatbuffers.Builder, consumerIdOffset: flatbuffers.Offset): void;
    static addProducerId(builder: flatbuffers.Builder, producerIdOffset: flatbuffers.Offset): void;
    static addKind(builder: flatbuffers.Builder, kind: MediaKind): void;
    static addRtpParameters(builder: flatbuffers.Builder, rtpParametersOffset: flatbuffers.Offset): void;
    static addType(builder: flatbuffers.Builder, type: Type): void;
    static addConsumableRtpEncodings(builder: flatbuffers.Builder, consumableRtpEncodingsOffset: flatbuffers.Offset): void;
    static createConsumableRtpEncodingsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startConsumableRtpEncodingsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addPaused(builder: flatbuffers.Builder, paused: boolean): void;
    static addPreferredLayers(builder: flatbuffers.Builder, preferredLayersOffset: flatbuffers.Offset): void;
    static addIgnoreDtx(builder: flatbuffers.Builder, ignoreDtx: boolean): void;
    static endConsumeRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): ConsumeRequestT;
    unpackTo(_o: ConsumeRequestT): void;
}
export declare class ConsumeRequestT {
    consumerId: string | Uint8Array | null;
    producerId: string | Uint8Array | null;
    kind: MediaKind;
    rtpParameters: RtpParametersT | null;
    type: Type;
    consumableRtpEncodings: (RtpEncodingParametersT)[];
    paused: boolean;
    preferredLayers: ConsumerLayersT | null;
    ignoreDtx: boolean;
    constructor(consumerId?: string | Uint8Array | null, producerId?: string | Uint8Array | null, kind?: MediaKind, rtpParameters?: RtpParametersT | null, type?: Type, consumableRtpEncodings?: (RtpEncodingParametersT)[], paused?: boolean, preferredLayers?: ConsumerLayersT | null, ignoreDtx?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=consume-request.d.ts.map