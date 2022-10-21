import * as flatbuffers from 'flatbuffers';
import { StringString, StringStringT } from '../../fbs/common/string-string';
import { Uint32String, Uint32StringT } from '../../fbs/common/uint32string';
export declare class RtpListener {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): RtpListener;
    static getRootAsRtpListener(bb: flatbuffers.ByteBuffer, obj?: RtpListener): RtpListener;
    static getSizePrefixedRootAsRtpListener(bb: flatbuffers.ByteBuffer, obj?: RtpListener): RtpListener;
    ssrcTable(index: number, obj?: Uint32String): Uint32String | null;
    ssrcTableLength(): number;
    midTable(index: number, obj?: StringString): StringString | null;
    midTableLength(): number;
    ridTable(index: number, obj?: StringString): StringString | null;
    ridTableLength(): number;
    static startRtpListener(builder: flatbuffers.Builder): void;
    static addSsrcTable(builder: flatbuffers.Builder, ssrcTableOffset: flatbuffers.Offset): void;
    static createSsrcTableVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startSsrcTableVector(builder: flatbuffers.Builder, numElems: number): void;
    static addMidTable(builder: flatbuffers.Builder, midTableOffset: flatbuffers.Offset): void;
    static createMidTableVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startMidTableVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRidTable(builder: flatbuffers.Builder, ridTableOffset: flatbuffers.Offset): void;
    static createRidTableVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRidTableVector(builder: flatbuffers.Builder, numElems: number): void;
    static endRtpListener(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createRtpListener(builder: flatbuffers.Builder, ssrcTableOffset: flatbuffers.Offset, midTableOffset: flatbuffers.Offset, ridTableOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): RtpListenerT;
    unpackTo(_o: RtpListenerT): void;
}
export declare class RtpListenerT {
    ssrcTable: (Uint32StringT)[];
    midTable: (StringStringT)[];
    ridTable: (StringStringT)[];
    constructor(ssrcTable?: (Uint32StringT)[], midTable?: (StringStringT)[], ridTable?: (StringStringT)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=rtp-listener.d.ts.map