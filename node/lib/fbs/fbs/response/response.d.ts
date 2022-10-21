import * as flatbuffers from 'flatbuffers';
import { Body } from '../../fbs/response/body';
import { RouterDumpT } from '../../fbs/router/router-dump';
import { ConsumeResponseT } from '../../fbs/transport/consume-response';
import { TransportDumpT } from '../../fbs/transport/transport-dump';
import { WebRtcServerDumpT } from '../../fbs/web-rtc-server/web-rtc-server-dump';
import { ResourceUsageT } from '../../fbs/worker/resource-usage';
import { WorkerDumpT } from '../../fbs/worker/worker-dump';
export declare class Response {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Response;
    static getRootAsResponse(bb: flatbuffers.ByteBuffer, obj?: Response): Response;
    static getSizePrefixedRootAsResponse(bb: flatbuffers.ByteBuffer, obj?: Response): Response;
    id(): number;
    accepted(): boolean;
    bodyType(): Body;
    body<T extends flatbuffers.Table>(obj: any): any | null;
    static startResponse(builder: flatbuffers.Builder): void;
    static addId(builder: flatbuffers.Builder, id: number): void;
    static addAccepted(builder: flatbuffers.Builder, accepted: boolean): void;
    static addBodyType(builder: flatbuffers.Builder, bodyType: Body): void;
    static addBody(builder: flatbuffers.Builder, bodyOffset: flatbuffers.Offset): void;
    static endResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createResponse(builder: flatbuffers.Builder, id: number, accepted: boolean, bodyType: Body, bodyOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): ResponseT;
    unpackTo(_o: ResponseT): void;
}
export declare class ResponseT {
    id: number;
    accepted: boolean;
    bodyType: Body;
    body: ConsumeResponseT | ResourceUsageT | RouterDumpT | TransportDumpT | WebRtcServerDumpT | WorkerDumpT | null;
    constructor(id?: number, accepted?: boolean, bodyType?: Body, body?: ConsumeResponseT | ResourceUsageT | RouterDumpT | TransportDumpT | WebRtcServerDumpT | WorkerDumpT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=response.d.ts.map