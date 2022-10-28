import * as flatbuffers from 'flatbuffers';
import { ChannelMessageHandlers, ChannelMessageHandlersT } from '../../fbs/worker/channel-message-handlers';
export declare class WorkerDumpResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): WorkerDumpResponse;
    static getRootAsWorkerDumpResponse(bb: flatbuffers.ByteBuffer, obj?: WorkerDumpResponse): WorkerDumpResponse;
    static getSizePrefixedRootAsWorkerDumpResponse(bb: flatbuffers.ByteBuffer, obj?: WorkerDumpResponse): WorkerDumpResponse;
    pid(): bigint;
    webrtcServerIds(index: number): string;
    webrtcServerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    webrtcServerIdsLength(): number;
    routerIds(index: number): string;
    routerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    routerIdsLength(): number;
    channelMessageHandlers(obj?: ChannelMessageHandlers): ChannelMessageHandlers | null;
    static startWorkerDumpResponse(builder: flatbuffers.Builder): void;
    static addPid(builder: flatbuffers.Builder, pid: bigint): void;
    static addWebrtcServerIds(builder: flatbuffers.Builder, webrtcServerIdsOffset: flatbuffers.Offset): void;
    static createWebrtcServerIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startWebrtcServerIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRouterIds(builder: flatbuffers.Builder, routerIdsOffset: flatbuffers.Offset): void;
    static createRouterIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRouterIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addChannelMessageHandlers(builder: flatbuffers.Builder, channelMessageHandlersOffset: flatbuffers.Offset): void;
    static endWorkerDumpResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): WorkerDumpResponseT;
    unpackTo(_o: WorkerDumpResponseT): void;
}
export declare class WorkerDumpResponseT {
    pid: bigint;
    webrtcServerIds: (string)[];
    routerIds: (string)[];
    channelMessageHandlers: ChannelMessageHandlersT | null;
    constructor(pid?: bigint, webrtcServerIds?: (string)[], routerIds?: (string)[], channelMessageHandlers?: ChannelMessageHandlersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=worker-dump-response.d.ts.map