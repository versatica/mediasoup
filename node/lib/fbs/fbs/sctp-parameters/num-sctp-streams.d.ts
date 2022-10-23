import * as flatbuffers from 'flatbuffers';
export declare class NumSctpStreams {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): NumSctpStreams;
    static getRootAsNumSctpStreams(bb: flatbuffers.ByteBuffer, obj?: NumSctpStreams): NumSctpStreams;
    static getSizePrefixedRootAsNumSctpStreams(bb: flatbuffers.ByteBuffer, obj?: NumSctpStreams): NumSctpStreams;
    os(): number;
    mis(): number;
    static startNumSctpStreams(builder: flatbuffers.Builder): void;
    static addOs(builder: flatbuffers.Builder, os: number): void;
    static addMis(builder: flatbuffers.Builder, mis: number): void;
    static endNumSctpStreams(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createNumSctpStreams(builder: flatbuffers.Builder, os: number, mis: number): flatbuffers.Offset;
    unpack(): NumSctpStreamsT;
    unpackTo(_o: NumSctpStreamsT): void;
}
export declare class NumSctpStreamsT {
    os: number;
    mis: number;
    constructor(os?: number, mis?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=num-sctp-streams.d.ts.map