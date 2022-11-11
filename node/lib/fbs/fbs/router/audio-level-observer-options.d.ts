import * as flatbuffers from 'flatbuffers';
export declare class AudioLevelObserverOptions {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): AudioLevelObserverOptions;
    static getRootAsAudioLevelObserverOptions(bb: flatbuffers.ByteBuffer, obj?: AudioLevelObserverOptions): AudioLevelObserverOptions;
    static getSizePrefixedRootAsAudioLevelObserverOptions(bb: flatbuffers.ByteBuffer, obj?: AudioLevelObserverOptions): AudioLevelObserverOptions;
    maxEntries(): number;
    threshold(): number;
    interval(): number;
    static startAudioLevelObserverOptions(builder: flatbuffers.Builder): void;
    static addMaxEntries(builder: flatbuffers.Builder, maxEntries: number): void;
    static addThreshold(builder: flatbuffers.Builder, threshold: number): void;
    static addInterval(builder: flatbuffers.Builder, interval: number): void;
    static endAudioLevelObserverOptions(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createAudioLevelObserverOptions(builder: flatbuffers.Builder, maxEntries: number, threshold: number, interval: number): flatbuffers.Offset;
    unpack(): AudioLevelObserverOptionsT;
    unpackTo(_o: AudioLevelObserverOptionsT): void;
}
export declare class AudioLevelObserverOptionsT {
    maxEntries: number;
    threshold: number;
    interval: number;
    constructor(maxEntries?: number, threshold?: number, interval?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=audio-level-observer-options.d.ts.map