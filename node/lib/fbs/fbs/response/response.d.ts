import * as flatbuffers from 'flatbuffers';
import { Body } from '../../fbs/response/body';
import { DumpResponseT as FBS_Router_DumpResponseT } from '../../fbs/router/dump-response';
import { ConsumeResponseT } from '../../fbs/transport/consume-response';
import { DumpResponseT as FBS_Transport_DumpResponseT } from '../../fbs/transport/dump-response';
import { DumpResponseT as FBS_WebRtcServer_DumpResponseT } from '../../fbs/web-rtc-server/dump-response';
import { DumpResponseT } from '../../fbs/worker/dump-response';
import { ResourceUsageResponseT } from '../../fbs/worker/resource-usage-response';
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
    body: ConsumeResponseT | DumpResponseT | FBS_Router_DumpResponseT | FBS_Transport_DumpResponseT | FBS_WebRtcServer_DumpResponseT | ResourceUsageResponseT | null;
    constructor(id?: number, accepted?: boolean, bodyType?: Body, body?: ConsumeResponseT | DumpResponseT | FBS_Router_DumpResponseT | FBS_Transport_DumpResponseT | FBS_WebRtcServer_DumpResponseT | ResourceUsageResponseT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=response.d.ts.map