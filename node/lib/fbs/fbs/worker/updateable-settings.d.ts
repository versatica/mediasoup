import * as flatbuffers from 'flatbuffers';
export declare class UpdateableSettings {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): UpdateableSettings;
    static getRootAsUpdateableSettings(bb: flatbuffers.ByteBuffer, obj?: UpdateableSettings): UpdateableSettings;
    static getSizePrefixedRootAsUpdateableSettings(bb: flatbuffers.ByteBuffer, obj?: UpdateableSettings): UpdateableSettings;
    logLevel(): string | null;
    logLevel(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    logTags(index: number): string;
    logTags(index: number, optionalEncoding: flatbuffers.Encoding): string | Uint8Array;
    logTagsLength(): number;
    static startUpdateableSettings(builder: flatbuffers.Builder): void;
    static addLogLevel(builder: flatbuffers.Builder, logLevelOffset: flatbuffers.Offset): void;
    static addLogTags(builder: flatbuffers.Builder, logTagsOffset: flatbuffers.Offset): void;
    static createLogTagsVector(builder: flatbuffers.Builder, data: flatbuffers.Offset[]): flatbuffers.Offset;
    static startLogTagsVector(builder: flatbuffers.Builder, numElems: number): void;
    static endUpdateableSettings(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createUpdateableSettings(builder: flatbuffers.Builder, logLevelOffset: flatbuffers.Offset, logTagsOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): UpdateableSettingsT;
    unpackTo(_o: UpdateableSettingsT): void;
}
export declare class UpdateableSettingsT {
    logLevel: string | Uint8Array | null;
    logTags: (string)[];
    constructor(logLevel?: string | Uint8Array | null, logTags?: (string)[]);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=updateable-settings.d.ts.map