import * as flatbuffers from 'flatbuffers';
import { Body } from '../../f-b-s/request/body';
export declare class Request {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): Request;
    static getRootAsRequest(bb: flatbuffers.ByteBuffer, obj?: Request): Request;
    static getSizePrefixedRootAsRequest(bb: flatbuffers.ByteBuffer, obj?: Request): Request;
    id(): number;
    handlerId(): string | null;
    handlerId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    bodyType(): Body;
    body<T extends flatbuffers.Table>(obj: any): any | null;
    static startRequest(builder: flatbuffers.Builder): void;
    static addId(builder: flatbuffers.Builder, id: number): void;
    static addHandlerId(builder: flatbuffers.Builder, handlerIdOffset: flatbuffers.Offset): void;
    static addBodyType(builder: flatbuffers.Builder, bodyType: Body): void;
    static addBody(builder: flatbuffers.Builder, bodyOffset: flatbuffers.Offset): void;
    static endRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static finishRequestBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static finishSizePrefixedRequestBuffer(builder: flatbuffers.Builder, offset: flatbuffers.Offset): void;
    static createRequest(builder: flatbuffers.Builder, id: number, handlerIdOffset: flatbuffers.Offset, bodyType: Body, bodyOffset: flatbuffers.Offset): flatbuffers.Offset;
}
//# sourceMappingURL=request.d.ts.map