import * as flatbuffers from 'flatbuffers';
export declare class IceCandidate {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): IceCandidate;
    static getRootAsIceCandidate(bb: flatbuffers.ByteBuffer, obj?: IceCandidate): IceCandidate;
    static getSizePrefixedRootAsIceCandidate(bb: flatbuffers.ByteBuffer, obj?: IceCandidate): IceCandidate;
    foundation(): string | null;
    foundation(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    priority(): number;
    ip(): string | null;
    ip(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    protocol(): string | null;
    protocol(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    port(): number;
    type(): string | null;
    type(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    tcpType(): string | null;
    tcpType(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startIceCandidate(builder: flatbuffers.Builder): void;
    static addFoundation(builder: flatbuffers.Builder, foundationOffset: flatbuffers.Offset): void;
    static addPriority(builder: flatbuffers.Builder, priority: number): void;
    static addIp(builder: flatbuffers.Builder, ipOffset: flatbuffers.Offset): void;
    static addProtocol(builder: flatbuffers.Builder, protocolOffset: flatbuffers.Offset): void;
    static addPort(builder: flatbuffers.Builder, port: number): void;
    static addType(builder: flatbuffers.Builder, typeOffset: flatbuffers.Offset): void;
    static addTcpType(builder: flatbuffers.Builder, tcpTypeOffset: flatbuffers.Offset): void;
    static endIceCandidate(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createIceCandidate(builder: flatbuffers.Builder, foundationOffset: flatbuffers.Offset, priority: number, ipOffset: flatbuffers.Offset, protocolOffset: flatbuffers.Offset, port: number, typeOffset: flatbuffers.Offset, tcpTypeOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): IceCandidateT;
    unpackTo(_o: IceCandidateT): void;
}
export declare class IceCandidateT {
    foundation: string | Uint8Array | null;
    priority: number;
    ip: string | Uint8Array | null;
    protocol: string | Uint8Array | null;
    port: number;
    type: string | Uint8Array | null;
    tcpType: string | Uint8Array | null;
    constructor(foundation?: string | Uint8Array | null, priority?: number, ip?: string | Uint8Array | null, protocol?: string | Uint8Array | null, port?: number, type?: string | Uint8Array | null, tcpType?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=ice-candidate.d.ts.map