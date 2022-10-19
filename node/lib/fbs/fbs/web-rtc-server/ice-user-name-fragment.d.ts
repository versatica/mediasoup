import * as flatbuffers from 'flatbuffers';
export declare class IceUserNameFragment {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): IceUserNameFragment;
    static getRootAsIceUserNameFragment(bb: flatbuffers.ByteBuffer, obj?: IceUserNameFragment): IceUserNameFragment;
    static getSizePrefixedRootAsIceUserNameFragment(bb: flatbuffers.ByteBuffer, obj?: IceUserNameFragment): IceUserNameFragment;
    localIceUsernameFragment(): string | null;
    localIceUsernameFragment(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    webRtcTransportId(): string | null;
    webRtcTransportId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startIceUserNameFragment(builder: flatbuffers.Builder): void;
    static addLocalIceUsernameFragment(builder: flatbuffers.Builder, localIceUsernameFragmentOffset: flatbuffers.Offset): void;
    static addWebRtcTransportId(builder: flatbuffers.Builder, webRtcTransportIdOffset: flatbuffers.Offset): void;
    static endIceUserNameFragment(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createIceUserNameFragment(builder: flatbuffers.Builder, localIceUsernameFragmentOffset: flatbuffers.Offset, webRtcTransportIdOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): IceUserNameFragmentT;
    unpackTo(_o: IceUserNameFragmentT): void;
}
export declare class IceUserNameFragmentT {
    localIceUsernameFragment: string | Uint8Array | null;
    webRtcTransportId: string | Uint8Array | null;
    constructor(localIceUsernameFragment?: string | Uint8Array | null, webRtcTransportId?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=ice-user-name-fragment.d.ts.map