import * as flatbuffers from 'flatbuffers';
import { ChannelMessageHandlers, ChannelMessageHandlersT } from '../../f-b-s/worker/channel-message-handlers';
export declare class Dump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Dump;
    static getRootAsDump(bb: flatbuffers.ByteBuffer, obj?: Dump): Dump;
    static getSizePrefixedRootAsDump(bb: flatbuffers.ByteBuffer, obj?: Dump): Dump;
    pid(): bigint;
    webrtcServerIds(index: number): string;
    webrtcServerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    webrtcServerIdsLength(): number;
    routerIds(index: number): string;
    routerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    routerIdsLength(): number;
    channelMessageHandlers(obj?: ChannelMessageHandlers): ChannelMessageHandlers | null;
    static startDump(builder: flatbuffers.Builder): void;
    static addPid(builder: flatbuffers.Builder, pid: bigint): void;
    static addWebrtcServerIds(builder: flatbuffers.Builder, webrtcServerIdsOffset: flatbuffers.Offset): void;
    static createWebrtcServerIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startWebrtcServerIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRouterIds(builder: flatbuffers.Builder, routerIdsOffset: flatbuffers.Offset): void;
    static createRouterIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRouterIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addChannelMessageHandlers(builder: flatbuffers.Builder, channelMessageHandlersOffset: flatbuffers.Offset): void;
    static endDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    static finishDumpBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static finishSizePrefixedDumpBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    unpack(): DumpT;
    unpackTo(_o: DumpT): void;
}
export declare class DumpT {
    pid: bigint;
    webrtcServerIds: (string)[];
    routerIds: (string)[];
    channelMessageHandlers: ChannelMessageHandlersT | null;
    constructor(pid?: bigint, webrtcServerIds?: (string)[], routerIds?: (string)[], channelMessageHandlers?: ChannelMessageHandlersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump.d.ts.map