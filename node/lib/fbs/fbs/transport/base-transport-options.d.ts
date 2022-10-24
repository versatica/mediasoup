import * as flatbuffers from 'flatbuffers';
import { NumSctpStreams, NumSctpStreamsT } from '../../fbs/sctp-parameters/num-sctp-streams';
export declare class BaseTransportOptions {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): BaseTransportOptions;
    static getRootAsBaseTransportOptions(bb: flatbuffers.ByteBuffer, obj?: BaseTransportOptions): BaseTransportOptions;
    static getSizePrefixedRootAsBaseTransportOptions(bb: flatbuffers.ByteBuffer, obj?: BaseTransportOptions): BaseTransportOptions;
    direct(): boolean;
    maxMessageSize(): number;
    initialAvailableOutgoingBitrate(): number;
    enableSctp(): boolean;
    numSctpStreams(obj?: NumSctpStreams): NumSctpStreams | null;
    maxSctpMessageSize(): number;
    sctpSendBufferSize(): number;
    isDataChannel(): boolean;
    static startBaseTransportOptions(builder: flatbuffers.Builder): void;
    static addDirect(builder: flatbuffers.Builder, direct: boolean): void;
    static addMaxMessageSize(builder: flatbuffers.Builder, maxMessageSize: number): void;
    static addInitialAvailableOutgoingBitrate(builder: flatbuffers.Builder, initialAvailableOutgoingBitrate: number): void;
    static addEnableSctp(builder: flatbuffers.Builder, enableSctp: boolean): void;
    static addNumSctpStreams(builder: flatbuffers.Builder, numSctpStreamsOffset: flatbuffers.Offset): void;
    static addMaxSctpMessageSize(builder: flatbuffers.Builder, maxSctpMessageSize: number): void;
    static addSctpSendBufferSize(builder: flatbuffers.Builder, sctpSendBufferSize: number): void;
    static addIsDataChannel(builder: flatbuffers.Builder, isDataChannel: boolean): void;
    static endBaseTransportOptions(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): BaseTransportOptionsT;
    unpackTo(_o: BaseTransportOptionsT): void;
}
export declare class BaseTransportOptionsT {
    direct: boolean;
    maxMessageSize: number;
    initialAvailableOutgoingBitrate: number;
    enableSctp: boolean;
    numSctpStreams: NumSctpStreamsT | null;
    maxSctpMessageSize: number;
    sctpSendBufferSize: number;
    isDataChannel: boolean;
    constructor(direct?: boolean, maxMessageSize?: number, initialAvailableOutgoingBitrate?: number, enableSctp?: boolean, numSctpStreams?: NumSctpStreamsT | null, maxSctpMessageSize?: number, sctpSendBufferSize?: number, isDataChannel?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=base-transport-options.d.ts.map