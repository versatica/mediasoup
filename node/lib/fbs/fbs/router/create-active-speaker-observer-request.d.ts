import * as flatbuffers from 'flatbuffers';
import { ActiveSpeakerObserverOptions, ActiveSpeakerObserverOptionsT } from '../../fbs/router/active-speaker-observer-options';
export declare class CreateActiveSpeakerObserverRequest {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): CreateActiveSpeakerObserverRequest;
    static getRootAsCreateActiveSpeakerObserverRequest(bb: flatbuffers.ByteBuffer, obj?: CreateActiveSpeakerObserverRequest): CreateActiveSpeakerObserverRequest;
    static getSizePrefixedRootAsCreateActiveSpeakerObserverRequest(bb: flatbuffers.ByteBuffer, obj?: CreateActiveSpeakerObserverRequest): CreateActiveSpeakerObserverRequest;
    activeSpeakerObserverId(): string | null;
    activeSpeakerObserverId(optionalEncoding: flatbuffers.Encoding): string | Uint8Array | null;
    options(obj?: ActiveSpeakerObserverOptions): ActiveSpeakerObserverOptions | null;
    static startCreateActiveSpeakerObserverRequest(builder: flatbuffers.Builder): void;
    static addActiveSpeakerObserverId(builder: flatbuffers.Builder, activeSpeakerObserverIdOffset: flatbuffers.Offset): void;
    static addOptions(builder: flatbuffers.Builder, optionsOffset: flatbuffers.Offset): void;
    static endCreateActiveSpeakerObserverRequest(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): CreateActiveSpeakerObserverRequestT;
    unpackTo(_o: CreateActiveSpeakerObserverRequestT): void;
}
export declare class CreateActiveSpeakerObserverRequestT {
    activeSpeakerObserverId: string | Uint8Array | null;
    options: ActiveSpeakerObserverOptionsT | null;
    constructor(activeSpeakerObserverId?: string | Uint8Array | null, options?: ActiveSpeakerObserverOptionsT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=create-active-speaker-observer-request.d.ts.map