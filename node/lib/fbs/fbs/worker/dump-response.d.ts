import * as flatbuffers from 'flatbuffers';
import { ChannelMessageHandlers, ChannelMessageHandlersT } from '../../fbs/worker/channel-message-handlers';
export declare class DumpResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DumpResponse;
    static getRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    static getSizePrefixedRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    pid(): number;
    webRtcServerIds(index: number): string;
    webRtcServerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    webRtcServerIdsLength(): number;
    routerIds(index: number): string;
    routerIds(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    routerIdsLength(): number;
    channelMessageHandlers(obj?: ChannelMessageHandlers): ChannelMessageHandlers | null;
    static startDumpResponse(builder: flatbuffers.Builder): void;
    static addPid(builder: flatbuffers.Builder, pid: number): void;
    static addWebRtcServerIds(builder: flatbuffers.Builder, webRtcServerIdsOffset: flatbuffers.Offset): void;
    static createWebRtcServerIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startWebRtcServerIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addRouterIds(builder: flatbuffers.Builder, routerIdsOffset: flatbuffers.Offset): void;
    static createRouterIdsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startRouterIdsVector(builder: flatbuffers.Builder, numElems: number): void;
    static addChannelMessageHandlers(builder: flatbuffers.Builder, channelMessageHandlersOffset: flatbuffers.Offset): void;
    static endDumpResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): DumpResponseT;
    unpackTo(_o: DumpResponseT): void;
}
export declare class DumpResponseT {
    pid: number;
    webRtcServerIds: (string)[];
    routerIds: (string)[];
    channelMessageHandlers: ChannelMessageHandlersT | null;
    constructor(pid?: number, webRtcServerIds?: (string)[], routerIds?: (string)[], channelMessageHandlers?: ChannelMessageHandlersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump-response.d.ts.map