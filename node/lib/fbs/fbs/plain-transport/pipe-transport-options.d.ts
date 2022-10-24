import * as flatbuffers from 'flatbuffers';
import { BaseTransportOptions, BaseTransportOptionsT } from '../../fbs/transport/base-transport-options';
import { TransportListenIp, TransportListenIpT } from '../../fbs/transport/transport-listen-ip';
export declare class PipeTransportOptions {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): PipeTransportOptions;
    static getRootAsPipeTransportOptions(bb: flatbuffers.ByteBuffer, obj?: PipeTransportOptions): PipeTransportOptions;
    static getSizePrefixedRootAsPipeTransportOptions(bb: flatbuffers.ByteBuffer, obj?: PipeTransportOptions): PipeTransportOptions;
    base(obj?: BaseTransportOptions): BaseTransportOptions | null;
    listenIp(obj?: TransportListenIp): TransportListenIp | null;
    enableRtx(): boolean;
    enableSrtp(): boolean;
    static startPipeTransportOptions(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addListenIp(builder: flatbuffers.Builder, listenIpOffset: flatbuffers.Offset): void;
    static addEnableRtx(builder: flatbuffers.Builder, enableRtx: boolean): void;
    static addEnableSrtp(builder: flatbuffers.Builder, enableSrtp: boolean): void;
    static endPipeTransportOptions(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): PipeTransportOptionsT;
    unpackTo(_o: PipeTransportOptionsT): void;
}
export declare class PipeTransportOptionsT {
    base: BaseTransportOptionsT | null;
    listenIp: TransportListenIpT | null;
    enableRtx: boolean;
    enableSrtp: boolean;
    constructor(base?: BaseTransportOptionsT | null, listenIp?: TransportListenIpT | null, enableRtx?: boolean, enableSrtp?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=pipe-transport-options.d.ts.map