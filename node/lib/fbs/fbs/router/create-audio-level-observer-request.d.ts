import * as flatbuffers from 'flatbuffers';
import { AudioLevelObserverOptions, AudioLevelObserverOptionsT } from '../../fbs/router/audio-level-observer-options';
export declare class CreateAudioLevelObserverRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CreateAudioLevelObserverRequest;
    static getRootAsCreateAudioLevelObserverRequest(bb: flatbuffers.ByteBuffer, obj?: CreateAudioLevelObserverRequest): CreateAudioLevelObserverRequest;
    static getSizePrefixedRootAsCreateAudioLevelObserverRequest(bb: flatbuffers.ByteBuffer, obj?: CreateAudioLevelObserverRequest): CreateAudioLevelObserverRequest;
    rtpObserverId(): string | null;
    rtpObserverId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    options(obj?: AudioLevelObserverOptions): AudioLevelObserverOptions | null;
    static startCreateAudioLevelObserverRequest(builder: flatbuffers.Builder): void;
    static addRtpObserverId(builder: flatbuffers.Builder, rtpObserverIdOffset: flatbuffers.Offset): void;
    static addOptions(builder: flatbuffers.Builder, optionsOffset: flatbuffers.Offset): void;
    static endCreateAudioLevelObserverRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): CreateAudioLevelObserverRequestT;
    unpackTo(_o: CreateAudioLevelObserverRequestT): void;
}
export declare class CreateAudioLevelObserverRequestT {
    rtpObserverId: string | Uint8Array | null;
    options: AudioLevelObserverOptionsT | null;
    constructor(rtpObserverId?: string | Uint8Array | null, options?: AudioLevelObserverOptionsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=create-audio-level-observer-request.d.ts.map