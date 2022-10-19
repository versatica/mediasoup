import * as flatbuffers from 'flatbuffers';
export declare class IpPort {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): IpPort;
    static getRootAsIpPort(bb: flatbuffers.ByteBuffer, obj?: IpPort): IpPort;
    static getSizePrefixedRootAsIpPort(bb: flatbuffers.ByteBuffer, obj?: IpPort): IpPort;
    ip(): string | null;
    ip(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    port(): number;
    static startIpPort(builder: flatbuffers.Builder): void;
    static addIp(builder: flatbuffers.Builder, ipOffset: flatbuffers.Offset): void;
    static addPort(builder: flatbuffers.Builder, port: number): void;
    static endIpPort(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createIpPort(builder: flatbuffers.Builder, ipOffset: flatbuffers.Offset, port: number): flatbuffers.Offset;
    unpack(): IpPortT;
    unpackTo(_o: IpPortT): void;
}
export declare class IpPortT {
    ip: string | Uint8Array | null;
    port: number;
    constructor(ip?: string | Uint8Array | null, port?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=ip-port.d.ts.map