import * as flatbuffers from 'flatbuffers';
import { Body } from '../../fbs/request/body';
import { Method } from '../../fbs/request/method';
import { CloseRtpObserverRequestT } from '../../fbs/router/close-rtp-observer-request';
import { CloseTransportRequestT } from '../../fbs/router/close-transport-request';
import { CreateActiveSpeakerObserverRequestT } from '../../fbs/router/create-active-speaker-observer-request';
import { CreateAudioLevelObserverRequestT } from '../../fbs/router/create-audio-level-observer-request';
import { CreateDirectTransportRequestT } from '../../fbs/router/create-direct-transport-request';
import { CreatePipeTransportRequestT } from '../../fbs/router/create-pipe-transport-request';
import { CreatePlainTransportRequestT } from '../../fbs/router/create-plain-transport-request';
import { CreateWebRtcTransportRequestT } from '../../fbs/router/create-web-rtc-transport-request';
import { CloseConsumerRequestT } from '../../fbs/transport/close-consumer-request';
import { CloseDataConsumerRequestT } from '../../fbs/transport/close-data-consumer-request';
import { CloseDataProducerRequestT } from '../../fbs/transport/close-data-producer-request';
import { CloseProducerRequestT } from '../../fbs/transport/close-producer-request';
import { ConsumeRequestT } from '../../fbs/transport/consume-request';
import { EnableTraceEventRequestT } from '../../fbs/transport/enable-trace-event-request';
import { SetMaxIncomingBitrateRequestT } from '../../fbs/transport/set-max-incoming-bitrate-request';
import { SetMaxOutgoingBitrateRequestT } from '../../fbs/transport/set-max-outgoing-bitrate-request';
import { CloseRouterRequestT } from '../../fbs/worker/close-router-request';
import { CloseWebRtcServerRequestT } from '../../fbs/worker/close-web-rtc-server-request';
import { CreateRouterRequestT } from '../../fbs/worker/create-router-request';
import { CreateWebRtcServerRequestT } from '../../fbs/worker/create-web-rtc-server-request';
import { UpdateSettingsRequestT } from '../../fbs/worker/update-settings-request';
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
    body: CloseConsumerRequestT | CloseDataConsumerRequestT | CloseDataProducerRequestT | CloseProducerRequestT | CloseRouterRequestT | CloseRtpObserverRequestT | CloseTransportRequestT | CloseWebRtcServerRequestT | ConsumeRequestT | CreateActiveSpeakerObserverRequestT | CreateAudioLevelObserverRequestT | CreateDirectTransportRequestT | CreatePipeTransportRequestT | CreatePlainTransportRequestT | CreateRouterRequestT | CreateWebRtcServerRequestT | CreateWebRtcTransportRequestT | EnableTraceEventRequestT | SetMaxIncomingBitrateRequestT | SetMaxOutgoingBitrateRequestT | UpdateSettingsRequestT | null;
    constructor(id?: number, method?: Method, handlerId?: string | Uint8Array | null, bodyType?: Body, body?: CloseConsumerRequestT | CloseDataConsumerRequestT | CloseDataProducerRequestT | CloseProducerRequestT | CloseRouterRequestT | CloseRtpObserverRequestT | CloseTransportRequestT | CloseWebRtcServerRequestT | ConsumeRequestT | CreateActiveSpeakerObserverRequestT | CreateAudioLevelObserverRequestT | CreateDirectTransportRequestT | CreatePipeTransportRequestT | CreatePlainTransportRequestT | CreateRouterRequestT | CreateWebRtcServerRequestT | CreateWebRtcTransportRequestT | EnableTraceEventRequestT | SetMaxIncomingBitrateRequestT | SetMaxOutgoingBitrateRequestT | UpdateSettingsRequestT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=request.d.ts.map