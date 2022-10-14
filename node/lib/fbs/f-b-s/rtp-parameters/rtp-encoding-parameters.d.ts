import * as flatbuffers from 'flatbuffers';
import { Rtx } from '../../f-b-s/rtp-parameters/rtx';
export declare class RtpEncodingParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RtpEncodingParameters;
    static getRootAsRtpEncodingParameters(bb: flatbuffers.ByteBuffer, obj?: RtpEncodingParameters): RtpEncodingParameters;
    static getSizePrefixedRootAsRtpEncodingParameters(bb: flatbuffers.ByteBuffer, obj?: RtpEncodingParameters): RtpEncodingParameters;
    ssrc(): number;
    rid(): string | null;
    rid(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    codecPayloadType(): number;
    rtx(obj?: Rtx): Rtx | null;
    dtx(): boolean;
    scalabilityMode(): string | null;
    scalabilityMode(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    scaleResolutionDownBy(): number;
    maxBitrate(): number;
    static startRtpEncodingParameters(builder: flatbuffers.Builder): void;
    static addSsrc(builder: flatbuffers.Builder, ssrc: number): void;
    static addRid(builder: flatbuffers.Builder, ridOffset: flatbuffers.Offset): void;
    static addCodecPayloadType(builder: flatbuffers.Builder, codecPayloadType: number): void;
    static addRtx(builder: flatbuffers.Builder, rtxOffset: flatbuffers.Offset): void;
    static addDtx(builder: flatbuffers.Builder, dtx: boolean): void;
    static addScalabilityMode(builder: flatbuffers.Builder, scalabilityModeOffset: flatbuffers.Offset): void;
    static addScaleResolutionDownBy(builder: flatbuffers.Builder, scaleResolutionDownBy: number): void;
    static addMaxBitrate(builder: flatbuffers.Builder, maxBitrate: number): void;
    static endRtpEncodingParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=rtp-encoding-parameters.d.ts.map