import * as flatbuffers from 'flatbuffers';
import { ChannelMessageHandlers, ChannelMessageHandlersT } from '../../fbs/worker/channel-message-handlers';
export declare class WorkerDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WorkerDump;
    static getRootAsWorkerDump(bb: flatbuffers.ByteBuffer, obj?: WorkerDump): WorkerDump;
    static getSizePrefixedRootAsWorkerDump(bb: flatbuffers.ByteBuffer, obj?: WorkerDump): WorkerDump;
    pid(): bigint;
    webrtcServerIds(index: number): string;
    webrtcServerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    webrtcServerIdsLength(): number;
    routerIds(index: number): string;
    routerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    routerIdsLength(): number;
    channelMessageHandlers(obj?: ChannelMessageHandlers): ChannelMessageHandlers | null;
    static startWorkerDump(builder: flatbuffers.Builder): void;
    static addPid(builder: flatbuffers.Builder, pid: bigint): void;
    static addWebrtcServerIds(builder: flatbuffers.Builder, webrtcServerIdsOffset: flatbuffers.Offset): void;
    static createWebrtcServerIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startWebrtcServerIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRouterIds(builder: flatbuffers.Builder, routerIdsOffset: flatbuffers.Offset): void;
    static createRouterIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRouterIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addChannelMessageHandlers(builder: flatbuffers.Builder, channelMessageHandlersOffset: flatbuffers.Offset): void;
    static endWorkerDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    static finishWorkerDumpBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static finishSizePrefixedWorkerDumpBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    unpack(): WorkerDumpT;
    unpackTo(_o: WorkerDumpT): void;
}
export declare class WorkerDumpT {
    pid: bigint;
    webrtcServerIds: (string)[];
    routerIds: (string)[];
    channelMessageHandlers: ChannelMessageHandlersT | null;
    constructor(pid?: bigint, webrtcServerIds?: (string)[], routerIds?: (string)[], channelMessageHandlers?: ChannelMessageHandlersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=worker-dump.d.ts.map