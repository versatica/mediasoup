import * as flatbuffers from 'flatbuffers';
export declare class TransportListenIp {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): TransportListenIp;
    static getRootAsTransportListenIp(bb: flatbuffers.ByteBuffer, obj?: TransportListenIp): TransportListenIp;
    static getSizePrefixedRootAsTransportListenIp(bb: flatbuffers.ByteBuffer, obj?: TransportListenIp): TransportListenIp;
    ip(): string | null;
    ip(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    announcedIp(): string | null;
    announcedIp(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startTransportListenIp(builder: flatbuffers.Builder): void;
    static addIp(builder: flatbuffers.Builder, ipOffset: flatbuffers.Offset): void;
    static addAnnouncedIp(builder: flatbuffers.Builder, announcedIpOffset: flatbuffers.Offset): void;
    static endTransportListenIp(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createTransportListenIp(builder: flatbuffers.Builder, ipOffset: flatbuffers.Offset, announcedIpOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): TransportListenIpT;
    unpackTo(_o: TransportListenIpT): void;
}
export declare class TransportListenIpT {
    ip: string | Uint8Array | null;
    announcedIp: string | Uint8Array | null;
    constructor(ip?: string | Uint8Array | null, announcedIp?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=transport-listen-ip.d.ts.map