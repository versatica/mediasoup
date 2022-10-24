import * as flatbuffers from 'flatbuffers';
import { BaseTransportOptions, BaseTransportOptionsT } from '../../fbs/transport/base-transport-options';
import { TransportListenIp, TransportListenIpT } from '../../fbs/transport/transport-listen-ip';
export declare class PlainTransportOptions {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): PlainTransportOptions;
    static getRootAsPlainTransportOptions(bb: flatbuffers.ByteBuffer, obj?: PlainTransportOptions): PlainTransportOptions;
    static getSizePrefixedRootAsPlainTransportOptions(bb: flatbuffers.ByteBuffer, obj?: PlainTransportOptions): PlainTransportOptions;
    base(obj?: BaseTransportOptions): BaseTransportOptions | null;
    listenIp(obj?: TransportListenIp): TransportListenIp | null;
    port(): number;
    rtcpMux(): boolean;
    comedia(): boolean;
    enableSrtp(): boolean;
    srtpCryptoSuite(): string | null;
    srtpCryptoSuite(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    static startPlainTransportOptions(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addListenIp(builder: flatbuffers.Builder, listenIpOffset: flatbuffers.Offset): void;
    static addPort(builder: flatbuffers.Builder, port: number): void;
    static addRtcpMux(builder: flatbuffers.Builder, rtcpMux: boolean): void;
    static addComedia(builder: flatbuffers.Builder, comedia: boolean): void;
    static addEnableSrtp(builder: flatbuffers.Builder, enableSrtp: boolean): void;
    static addSrtpCryptoSuite(builder: flatbuffers.Builder, srtpCryptoSuiteOffset: flatbuffers.Offset): void;
    static endPlainTransportOptions(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): PlainTransportOptionsT;
    unpackTo(_o: PlainTransportOptionsT): void;
}
export declare class PlainTransportOptionsT {
    base: BaseTransportOptionsT | null;
    listenIp: TransportListenIpT | null;
    port: number;
    rtcpMux: boolean;
    comedia: boolean;
    enableSrtp: boolean;
    srtpCryptoSuite: string | Uint8Array | null;
    constructor(base?: BaseTransportOptionsT | null, listenIp?: TransportListenIpT | null, port?: number, rtcpMux?: boolean, comedia?: boolean, enableSrtp?: boolean, srtpCryptoSuite?: string | Uint8Array | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=plain-transport-options.d.ts.map