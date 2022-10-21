import * as flatbuffers from 'flatbuffers';
export declare class SctpParameters {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SctpParameters;
    static getRootAsSctpParameters(bb: flatbuffers.ByteBuffer, obj?: SctpParameters): SctpParameters;
    static getSizePrefixedRootAsSctpParameters(bb: flatbuffers.ByteBuffer, obj?: SctpParameters): SctpParameters;
    port(): number;
    os(): number;
    mis(): number;
    maxMessageSize(): number;
    sendBufferSize(): number;
    sctpBufferedAmount(): number;
    isDataChannel(): boolean;
    static startSctpParameters(builder: flatbuffers.Builder): void;
    static addPort(builder: flatbuffers.Builder, port: number): void;
    static addOs(builder: flatbuffers.Builder, os: number): void;
    static addMis(builder: flatbuffers.Builder, mis: number): void;
    static addMaxMessageSize(builder: flatbuffers.Builder, maxMessageSize: number): void;
    static addSendBufferSize(builder: flatbuffers.Builder, sendBufferSize: number): void;
    static addSctpBufferedAmount(builder: flatbuffers.Builder, sctpBufferedAmount: number): void;
    static addIsDataChannel(builder: flatbuffers.Builder, isDataChannel: boolean): void;
    static endSctpParameters(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSctpParameters(builder: flatbuffers.Builder, port: number, os: number, mis: number, maxMessageSize: number, sendBufferSize: number, sctpBufferedAmount: number, isDataChannel: boolean): flatbuffers.Offset;
    unpack(): SctpParametersT;
    unpackTo(_o: SctpParametersT): void;
}
export declare class SctpParametersT {
    port: number;
    os: number;
    mis: number;
    maxMessageSize: number;
    sendBufferSize: number;
    sctpBufferedAmount: number;
    isDataChannel: boolean;
    constructor(port?: number, os?: number, mis?: number, maxMessageSize?: number, sendBufferSize?: number, sctpBufferedAmount?: number, isDataChannel?: boolean);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=sctp-parameters.d.ts.map