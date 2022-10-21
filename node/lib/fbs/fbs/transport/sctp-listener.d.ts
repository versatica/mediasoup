import * as flatbuffers from 'flatbuffers';
import { Uint16String, Uint16StringT } from '../../fbs/transport/uint16string';
export declare class SctpListener {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SctpListener;
    static getRootAsSctpListener(bb: flatbuffers.ByteBuffer, obj?: SctpListener): SctpListener;
    static getSizePrefixedRootAsSctpListener(bb: flatbuffers.ByteBuffer, obj?: SctpListener): SctpListener;
    streamIdTable(index: number, obj?: Uint16String): Uint16String | null;
    streamIdTableLength(): number;
    static startSctpListener(builder: flatbuffers.Builder): void;
    static addStreamIdTable(builder: flatbuffers.Builder, streamIdTableOffset: flatbuffers.Offset): void;
    static createStreamIdTableVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startStreamIdTableVector(builder: flatbuffers.Builder, numElems: number): void;
    static endSctpListener(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSctpListener(builder: flatbuffers.Builder, streamIdTableOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): SctpListenerT;
    unpackTo(_o: SctpListenerT): void;
}
export declare class SctpListenerT {
    streamIdTable: (Uint16StringT)[];
    constructor(streamIdTable?: (Uint16StringT)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=sctp-listener.d.ts.map