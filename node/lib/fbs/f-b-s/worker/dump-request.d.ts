import * as flatbuffers from 'flatbuffers';
export declare class DumpRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DumpRequest;
    static getRootAsDumpRequest(bb: flatbuffers.ByteBuffer, obj?: DumpRequest): DumpRequest;
    static getSizePrefixedRootAsDumpRequest(bb: flatbuffers.ByteBuffer, obj?: DumpRequest): DumpRequest;
    static startDumpRequest(builder: flatbuffers.Builder): void;
    static endDumpRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDumpRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump-request.d.ts.map