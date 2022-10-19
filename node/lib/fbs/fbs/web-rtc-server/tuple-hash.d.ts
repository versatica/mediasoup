import * as flatbuffers from 'flatbuffers';
export declare class TupleHash {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): TupleHash;
    static getRootAsTupleHash(bb: flatbuffers.ByteBuffer, obj?: TupleHash): TupleHash;
    static getSizePrefixedRootAsTupleHash(bb: flatbuffers.ByteBuffer, obj?: TupleHash): TupleHash;
    localIceUsernameFragment(): bigint;
    webRtcTransportId(): string | null;
    webRtcTransportId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startTupleHash(builder: flatbuffers.Builder): void;
    static addLocalIceUsernameFragment(builder: flatbuffers.Builder, localIceUsernameFragment: bigint): void;
    static addWebRtcTransportId(builder: flatbuffers.Builder, webRtcTransportIdOffset: flatbuffers.Offset): void;
    static endTupleHash(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createTupleHash(builder: flatbuffers.Builder, localIceUsernameFragment: bigint, webRtcTransportIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): TupleHashT;
    unpackTo(_o: TupleHashT): void;
}
export declare class TupleHashT {
    localIceUsernameFragment: bigint;
    webRtcTransportId: string | Uint8Array | null;
    constructor(localIceUsernameFragment?: bigint, webRtcTransportId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=tuple-hash.d.ts.map