import * as flatbuffers from 'flatbuffers';
import { CodecMapping, CodecMappingT } from '../../fbs/rtp-parameters/codec-mapping';
import { EncodingMapping, EncodingMappingT } from '../../fbs/rtp-parameters/encoding-mapping';
export declare class RtpMapping {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RtpMapping;
    static getRootAsRtpMapping(bb: flatbuffers.ByteBuffer, obj?: RtpMapping): RtpMapping;
    static getSizePrefixedRootAsRtpMapping(bb: flatbuffers.ByteBuffer, obj?: RtpMapping): RtpMapping;
    codecs(index: number, obj?: CodecMapping): CodecMapping | null;
    codecsLength(): number;
    encodings(index: number, obj?: EncodingMapping): EncodingMapping | null;
    encodingsLength(): number;
    static startRtpMapping(builder: flatbuffers.Builder): void;
    static addCodecs(builder: flatbuffers.Builder, codecsOffset: flatbuffers.Offset): void;
    static createCodecsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startCodecsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addEncodings(builder: flatbuffers.Builder, encodingsOffset: flatbuffers.Offset): void;
    static createEncodingsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startEncodingsVector(builder: flatbuffers.Builder, numElems: number): void;
    static endRtpMapping(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createRtpMapping(builder: flatbuffers.Builder, codecsOffset: flatbuffers.Offset, encodingsOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): RtpMappingT;
    unpackTo(_o: RtpMappingT): void;
}
export declare class RtpMappingT {
    codecs: (CodecMappingT)[];
    encodings: (EncodingMappingT)[];
    constructor(codecs?: (CodecMappingT)[], encodings?: (EncodingMappingT)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=rtp-mapping.d.ts.map