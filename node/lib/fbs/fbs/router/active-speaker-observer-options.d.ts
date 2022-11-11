import * as flatbuffers from 'flatbuffers';
export declare class ActiveSpeakerObserverOptions {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): ActiveSpeakerObserverOptions;
    static getRootAsActiveSpeakerObserverOptions(bb: flatbuffers.ByteBuffer, obj?: ActiveSpeakerObserverOptions): ActiveSpeakerObserverOptions;
    static getSizePrefixedRootAsActiveSpeakerObserverOptions(bb: flatbuffers.ByteBuffer, obj?: ActiveSpeakerObserverOptions): ActiveSpeakerObserverOptions;
    interval(): number;
    static startActiveSpeakerObserverOptions(builder: flatbuffers.Builder): void;
    static addInterval(builder: flatbuffers.Builder, interval: number): void;
    static endActiveSpeakerObserverOptions(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createActiveSpeakerObserverOptions(builder: flatbuffers.Builder, interval: number): flatbuffers.Offset;
    unpack(): ActiveSpeakerObserverOptionsT;
    unpackTo(_o: ActiveSpeakerObserverOptionsT): void;
}
export declare class ActiveSpeakerObserverOptionsT {
    interval: number;
    constructor(interval?: number);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=active-speaker-observer-options.d.ts.map