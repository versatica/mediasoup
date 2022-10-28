import * as flatbuffers from 'flatbuffers';
export declare class UpdateSettingsRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): UpdateSettingsRequest;
    static getRootAsUpdateSettingsRequest(bb: flatbuffers.ByteBuffer, obj?: UpdateSettingsRequest): UpdateSettingsRequest;
    static getSizePrefixedRootAsUpdateSettingsRequest(bb: flatbuffers.ByteBuffer, obj?: UpdateSettingsRequest): UpdateSettingsRequest;
    logLevel(): string | null;
    logLevel(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    logTags(index: number): string;
    logTags(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    logTagsLength(): number;
    static startUpdateSettingsRequest(builder: flatbuffers.Builder): void;
    static addLogLevel(builder: flatbuffers.Builder, logLevelOffset: flatbuffers.Offset): void;
    static addLogTags(builder: flatbuffers.Builder, logTagsOffset: flatbuffers.Offset): void;
    static createLogTagsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startLogTagsVector(builder: flatbuffers.Builder, numElems: number): void;
    static endUpdateSettingsRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createUpdateSettingsRequest(builder: flatbuffers.Builder, logLevelOffset: flatbuffers.Offset, logTagsOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): UpdateSettingsRequestT;
    unpackTo(_o: UpdateSettingsRequestT): void;
}
export declare class UpdateSettingsRequestT {
    logLevel: string | Uint8Array | null;
    logTags: (string)[];
    constructor(logLevel?: string | Uint8Array | null, logTags?: (string)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=update-settings-request.d.ts.map