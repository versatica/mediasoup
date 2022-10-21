import * as flatbuffers from 'flatbuffers';
export declare class IceParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): IceParameters;
    static getRootAsIceParameters(bb: flatbuffers.ByteBuffer, obj?: IceParameters): IceParameters;
    static getSizePrefixedRootAsIceParameters(bb: flatbuffers.ByteBuffer, obj?: IceParameters): IceParameters;
    usernameFragment(): string | null;
    usernameFragment(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    password(): string | null;
    password(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    iceLite(): boolean;
    static startIceParameters(builder: flatbuffers.Builder): void;
    static addUsernameFragment(builder: flatbuffers.Builder, usernameFragmentOffset: flatbuffers.Offset): void;
    static addPassword(builder: flatbuffers.Builder, passwordOffset: flatbuffers.Offset): void;
    static addIceLite(builder: flatbuffers.Builder, iceLite: boolean): void;
    static endIceParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createIceParameters(builder: flatbuffers.Builder, usernameFragmentOffset: flatbuffers.Offset, passwordOffset: flatbuffers.Offset, iceLite: boolean): flatbuffers.Offset;
    unpack(): IceParametersT;
    unpackTo(_o: IceParametersT): void;
}
export declare class IceParametersT {
    usernameFragment: string | Uint8Array | null;
    password: string | Uint8Array | null;
    iceLite: boolean;
    constructor(usernameFragment?: string | Uint8Array | null, password?: string | Uint8Array | null, iceLite?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=ice-parameters.d.ts.map