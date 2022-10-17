import * as flatbuffers from 'flatbuffers';
import { Body } from '../../f-b-s/request/body';
import { Method } from '../../f-b-s/request/method';
import { ConsumeRequestT } from '../../f-b-s/transport/consume-request';
import { UpdateableSettingsT } from '../../f-b-s/worker/updateable-settings';
export declare class Request {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Request;
    static getRootAsRequest(bb: flatbuffers.ByteBuffer, obj?: Request): Request;
    static getSizePrefixedRootAsRequest(bb: flatbuffers.ByteBuffer, obj?: Request): Request;
    id(): number;
    method(): Method;
    handlerId(): string | null;
    handlerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    bodyType(): Body;
    body<T extends flatbuffers.Table>(obj: any): any | null;
    static startRequest(builder: flatbuffers.Builder): void;
    static addId(builder: flatbuffers.Builder, id: number): void;
    static addMethod(builder: flatbuffers.Builder, method: Method): void;
    static addHandlerId(builder: flatbuffers.Builder, handlerIdOffset: flatbuffers.Offset): void;
    static addBodyType(builder: flatbuffers.Builder, bodyType: Body): void;
    static addBody(builder: flatbuffers.Builder, bodyOffset: flatbuffers.Offset): void;
    static endRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static finishRequestBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static finishSizePrefixedRequestBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static createRequest(builder: flatbuffers.Builder, id: number, method: Method, handlerIdOffset: flatbuffers.Offset, bodyType: Body, bodyOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): RequestT;
    unpackTo(_o: RequestT): void;
}
export declare class RequestT {
    id: number;
    method: Method;
    handlerId: string | Uint8Array | null;
    bodyType: Body;
    body: ConsumeRequestT | UpdateableSettingsT | null;
    constructor(id?: number, method?: Method, handlerId?: string | Uint8Array | null, bodyType?: Body, body?: ConsumeRequestT | UpdateableSettingsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=request.d.ts.map