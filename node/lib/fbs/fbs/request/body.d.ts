import { EnableTraceEventRequest as FBS_Consumer_EnableTraceEventRequest } from '../../fbs/consumer/enable-trace-event-request';
import { SetPreferredLayersRequest } from '../../fbs/consumer/set-preferred-layers-request';
import { EnableTraceEventRequest as FBS_Producer_EnableTraceEventRequest } from '../../fbs/producer/enable-trace-event-request';
import { CloseRtpObserverRequest } from '../../fbs/router/close-rtp-observer-request';
import { CloseTransportRequest } from '../../fbs/router/close-transport-request';
import { CreateActiveSpeakerObserverRequest } from '../../fbs/router/create-active-speaker-observer-request';
import { CreateAudioLevelObserverRequest } from '../../fbs/router/create-audio-level-observer-request';
import { CreateDirectTransportRequest } from '../../fbs/router/create-direct-transport-request';
import { CreatePipeTransportRequest } from '../../fbs/router/create-pipe-transport-request';
import { CreatePlainTransportRequest } from '../../fbs/router/create-plain-transport-request';
import { CreateWebRtcTransportRequest } from '../../fbs/router/create-web-rtc-transport-request';
import { AddProducerRequest } from '../../fbs/rtp-observer/add-producer-request';
import { RemoveProducerRequest } from '../../fbs/rtp-observer/remove-producer-request';
import { CloseConsumerRequest } from '../../fbs/transport/close-consumer-request';
import { CloseDataConsumerRequest } from '../../fbs/transport/close-data-consumer-request';
import { CloseDataProducerRequest } from '../../fbs/transport/close-data-producer-request';
import { CloseProducerRequest } from '../../fbs/transport/close-producer-request';
import { ConsumeRequest } from '../../fbs/transport/consume-request';
import { EnableTraceEventRequest } from '../../fbs/transport/enable-trace-event-request';
import { ProduceRequest } from '../../fbs/transport/produce-request';
import { SetMaxIncomingBitrateRequest } from '../../fbs/transport/set-max-incoming-bitrate-request';
import { SetMaxOutgoingBitrateRequest } from '../../fbs/transport/set-max-outgoing-bitrate-request';
import { CloseRouterRequest } from '../../fbs/worker/close-router-request';
import { CloseWebRtcServerRequest } from '../../fbs/worker/close-web-rtc-server-request';
import { CreateRouterRequest } from '../../fbs/worker/create-router-request';
import { CreateWebRtcServerRequest } from '../../fbs/worker/create-web-rtc-server-request';
import { UpdateSettingsRequest } from '../../fbs/worker/update-settings-request';
export declare enum Body {
    NONE = 0,
    FBS_Worker_UpdateSettingsRequest = 1,
    FBS_Worker_CreateWebRtcServerRequest = 2,
    FBS_Worker_CloseWebRtcServerRequest = 3,
    FBS_Worker_CreateRouterRequest = 4,
    FBS_Worker_CloseRouterRequest = 5,
    FBS_Router_CreateWebRtcTransportRequest = 6,
    FBS_Router_CreatePlainTransportRequest = 7,
    FBS_Router_CreatePipeTransportRequest = 8,
    FBS_Router_CreateDirectTransportRequest = 9,
    FBS_Router_CreateActiveSpeakerObserverRequest = 10,
    FBS_Router_CreateAudioLevelObserverRequest = 11,
    FBS_Router_CloseTransportRequest = 12,
    FBS_Router_CloseRtpObserverRequest = 13,
    FBS_Transport_SetMaxIncomingBitrateRequest = 14,
    FBS_Transport_SetMaxOutgoingBitrateRequest = 15,
    FBS_Transport_ProduceRequest = 16,
    FBS_Transport_ConsumeRequest = 17,
    FBS_Transport_EnableTraceEventRequest = 18,
    FBS_Transport_CloseProducerRequest = 19,
    FBS_Transport_CloseConsumerRequest = 20,
    FBS_Transport_CloseDataProducerRequest = 21,
    FBS_Transport_CloseDataConsumerRequest = 22,
    FBS_Producer_EnableTraceEventRequest = 23,
    FBS_Consumer_SetPreferredLayersRequest = 24,
    FBS_Consumer_EnableTraceEventRequest = 25,
    FBS_RtpObserver_AddProducerRequest = 26,
    FBS_RtpObserver_RemoveProducerRequest = 27
}
export declare function unionToBody(type: Body, accessor: (obj: AddProducerRequest | CloseConsumerRequest | CloseDataConsumerRequest | CloseDataProducerRequest | CloseProducerRequest | CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | EnableTraceEventRequest | FBS_Consumer_EnableTraceEventRequest | FBS_Producer_EnableTraceEventRequest | ProduceRequest | RemoveProducerRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | SetPreferredLayersRequest | UpdateSettingsRequest) => AddProducerRequest | CloseConsumerRequest | CloseDataConsumerRequest | CloseDataProducerRequest | CloseProducerRequest | CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | EnableTraceEventRequest | FBS_Consumer_EnableTraceEventRequest | FBS_Producer_EnableTraceEventRequest | ProduceRequest | RemoveProducerRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | SetPreferredLayersRequest | UpdateSettingsRequest | null): AddProducerRequest | CloseConsumerRequest | CloseDataConsumerRequest | CloseDataProducerRequest | CloseProducerRequest | CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | EnableTraceEventRequest | FBS_Consumer_EnableTraceEventRequest | FBS_Producer_EnableTraceEventRequest | ProduceRequest | RemoveProducerRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | SetPreferredLayersRequest | UpdateSettingsRequest | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: AddProducerRequest | CloseConsumerRequest | CloseDataConsumerRequest | CloseDataProducerRequest | CloseProducerRequest | CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | EnableTraceEventRequest | FBS_Consumer_EnableTraceEventRequest | FBS_Producer_EnableTraceEventRequest | ProduceRequest | RemoveProducerRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | SetPreferredLayersRequest | UpdateSettingsRequest) => AddProducerRequest | CloseConsumerRequest | CloseDataConsumerRequest | CloseDataProducerRequest | CloseProducerRequest | CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | EnableTraceEventRequest | FBS_Consumer_EnableTraceEventRequest | FBS_Producer_EnableTraceEventRequest | ProduceRequest | RemoveProducerRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | SetPreferredLayersRequest | UpdateSettingsRequest | null, index: number): AddProducerRequest | CloseConsumerRequest | CloseDataConsumerRequest | CloseDataProducerRequest | CloseProducerRequest | CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | EnableTraceEventRequest | FBS_Consumer_EnableTraceEventRequest | FBS_Producer_EnableTraceEventRequest | ProduceRequest | RemoveProducerRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | SetPreferredLayersRequest | UpdateSettingsRequest | null;
//# sourceMappingURL=body.d.ts.map