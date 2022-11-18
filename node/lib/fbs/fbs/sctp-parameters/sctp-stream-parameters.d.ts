import * as flatbuffers from 'flatbuffers';
export declare class SctpStreamParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SctpStreamParameters;
    static getRootAsSctpStreamParameters(bb: flatbuffers.ByteBuffer, obj?: SctpStreamParameters): SctpStreamParameters;
    static getSizePrefixedRootAsSctpStreamParameters(bb: flatbuffers.ByteBuffer, obj?: SctpStreamParameters): SctpStreamParameters;
    streamId(): number;
    ordered(): boolean | null;
    maxPacketLifeTime(): number | null;
    maxRetransmits(): number | null;
    static startSctpStreamParameters(builder: flatbuffers.Builder): void;
    static addStreamId(builder: flatbuffers.Builder, streamId: number): void;
    static addOrdered(builder: flatbuffers.Builder, ordered: boolean): void;
    static addMaxPacketLifeTime(builder: flatbuffers.Builder, maxPacketLifeTime: number): void;
    static addMaxRetransmits(builder: flatbuffers.Builder, maxRetransmits: number): void;
    static endSctpStreamParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSctpStreamParameters(builder: flatbuffers.Builder, streamId: number, ordered: boolean | null, maxPacketLifeTime: number | null, maxRetransmits: number | null): flatbuffers.Offset;
    unpack(): SctpStreamParametersT;
    unpackTo(_o: SctpStreamParametersT): void;
}
export declare class SctpStreamParametersT {
    streamId: number;
    ordered: boolean | null;
    maxPacketLifeTime: number | null;
    maxRetransmits: number | null;
    constructor(streamId?: number, ordered?: boolean | null, maxPacketLifeTime?: number | null, maxRetransmits?: number | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=sctp-stream-parameters.d.ts.map