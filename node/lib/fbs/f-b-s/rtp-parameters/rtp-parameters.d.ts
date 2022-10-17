import * as flatbuffers from 'flatbuffers';
import { RtcpParameters, RtcpParametersT } from '../../f-b-s/rtp-parameters/rtcp-parameters';
import { RtpCodecParameters, RtpCodecParametersT } from '../../f-b-s/rtp-parameters/rtp-codec-parameters';
import { RtpEncodingParameters, RtpEncodingParametersT } from '../../f-b-s/rtp-parameters/rtp-encoding-parameters';
import { RtpHeaderExtensionParameters, RtpHeaderExtensionParametersT } from '../../f-b-s/rtp-parameters/rtp-header-extension-parameters';
export declare class RtpParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RtpParameters;
    static getRootAsRtpParameters(bb: flatbuffers.ByteBuffer, obj?: RtpParameters): RtpParameters;
    static getSizePrefixedRootAsRtpParameters(bb: flatbuffers.ByteBuffer, obj?: RtpParameters): RtpParameters;
    mid(): string | null;
    mid(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    codecs(index: number, obj?: RtpCodecParameters): RtpCodecParameters | null;
    codecsLength(): number;
    headerExtensions(index: number, obj?: RtpHeaderExtensionParameters): RtpHeaderExtensionParameters | null;
    headerExtensionsLength(): number;
    encodings(index: number, obj?: RtpEncodingParameters): RtpEncodingParameters | null;
    encodingsLength(): number;
    rtcp(obj?: RtcpParameters): RtcpParameters | null;
    static startRtpParameters(builder: flatbuffers.Builder): void;
    static addMid(builder: flatbuffers.Builder, midOffset: flatbuffers.Offset): void;
    static addCodecs(builder: flatbuffers.Builder, codecsOffset: flatbuffers.Offset): void;
    static createCodecsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startCodecsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addHeaderExtensions(builder: flatbuffers.Builder, headerExtensionsOffset: flatbuffers.Offset): void;
    static createHeaderExtensionsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startHeaderExtensionsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addEncodings(builder: flatbuffers.Builder, encodingsOffset: flatbuffers.Offset): void;
    static createEncodingsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startEncodingsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRtcp(builder: flatbuffers.Builder, rtcpOffset: flatbuffers.Offset): void;
    static endRtpParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static finishRtpParametersBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static finishSizePrefixedRtpParametersBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    unpack(): RtpParametersT;
    unpackTo(_o: RtpParametersT): void;
}
export declare class RtpParametersT {
    mid: string | Uint8Array | null;
    codecs: (RtpCodecParametersT)[];
    headerExtensions: (RtpHeaderExtensionParametersT)[];
    encodings: (RtpEncodingParametersT)[];
    rtcp: RtcpParametersT | null;
    constructor(mid?: string | Uint8Array | null, codecs?: (RtpCodecParametersT)[], headerExtensions?: (RtpHeaderExtensionParametersT)[], encodings?: (RtpEncodingParametersT)[], rtcp?: RtcpParametersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=rtp-parameters.d.ts.map