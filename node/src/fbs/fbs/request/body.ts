// automatically generated by the FlatBuffers compiler, do not modify

import { EnableTraceEventRequest as FBS_Consumer_EnableTraceEventRequest, EnableTraceEventRequestT as FBS_Consumer_EnableTraceEventRequestT } from '../../fbs/consumer/enable-trace-event-request';
import { SetPreferredLayersRequest, SetPreferredLayersRequestT } from '../../fbs/consumer/set-preferred-layers-request';
import { SetPriorityRequest, SetPriorityRequestT } from '../../fbs/consumer/set-priority-request';
import { SetBufferedAmountLowThresholdRequest, SetBufferedAmountLowThresholdRequestT } from '../../fbs/data-consumer/set-buffered-amount-low-threshold-request';
import { EnableTraceEventRequest as FBS_Producer_EnableTraceEventRequest, EnableTraceEventRequestT as FBS_Producer_EnableTraceEventRequestT } from '../../fbs/producer/enable-trace-event-request';
import { CloseRtpObserverRequest, CloseRtpObserverRequestT } from '../../fbs/router/close-rtp-observer-request';
import { CloseTransportRequest, CloseTransportRequestT } from '../../fbs/router/close-transport-request';
import { CreateActiveSpeakerObserverRequest, CreateActiveSpeakerObserverRequestT } from '../../fbs/router/create-active-speaker-observer-request';
import { CreateAudioLevelObserverRequest, CreateAudioLevelObserverRequestT } from '../../fbs/router/create-audio-level-observer-request';
import { CreateDirectTransportRequest, CreateDirectTransportRequestT } from '../../fbs/router/create-direct-transport-request';
import { CreatePipeTransportRequest, CreatePipeTransportRequestT } from '../../fbs/router/create-pipe-transport-request';
import { CreatePlainTransportRequest, CreatePlainTransportRequestT } from '../../fbs/router/create-plain-transport-request';
import { CreateWebRtcTransportRequest, CreateWebRtcTransportRequestT } from '../../fbs/router/create-web-rtc-transport-request';
import { AddProducerRequest, AddProducerRequestT } from '../../fbs/rtp-observer/add-producer-request';
import { RemoveProducerRequest, RemoveProducerRequestT } from '../../fbs/rtp-observer/remove-producer-request';
import { CloseConsumerRequest, CloseConsumerRequestT } from '../../fbs/transport/close-consumer-request';
import { CloseDataConsumerRequest, CloseDataConsumerRequestT } from '../../fbs/transport/close-data-consumer-request';
import { CloseDataProducerRequest, CloseDataProducerRequestT } from '../../fbs/transport/close-data-producer-request';
import { CloseProducerRequest, CloseProducerRequestT } from '../../fbs/transport/close-producer-request';
import { ConsumeDataRequest, ConsumeDataRequestT } from '../../fbs/transport/consume-data-request';
import { ConsumeRequest, ConsumeRequestT } from '../../fbs/transport/consume-request';
import { EnableTraceEventRequest, EnableTraceEventRequestT } from '../../fbs/transport/enable-trace-event-request';
import { ProduceDataRequest, ProduceDataRequestT } from '../../fbs/transport/produce-data-request';
import { ProduceRequest, ProduceRequestT } from '../../fbs/transport/produce-request';
import { SetMaxIncomingBitrateRequest, SetMaxIncomingBitrateRequestT } from '../../fbs/transport/set-max-incoming-bitrate-request';
import { SetMaxOutgoingBitrateRequest, SetMaxOutgoingBitrateRequestT } from '../../fbs/transport/set-max-outgoing-bitrate-request';
import { CloseRouterRequest, CloseRouterRequestT } from '../../fbs/worker/close-router-request';
import { CloseWebRtcServerRequest, CloseWebRtcServerRequestT } from '../../fbs/worker/close-web-rtc-server-request';
import { CreateRouterRequest, CreateRouterRequestT } from '../../fbs/worker/create-router-request';
import { CreateWebRtcServerRequest, CreateWebRtcServerRequestT } from '../../fbs/worker/create-web-rtc-server-request';
import { UpdateSettingsRequest, UpdateSettingsRequestT } from '../../fbs/worker/update-settings-request';


export enum Body {
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
  FBS_Transport_ProduceDataRequest = 18,
  FBS_Transport_ConsumeDataRequest = 19,
  FBS_Transport_EnableTraceEventRequest = 20,
  FBS_Transport_CloseProducerRequest = 21,
  FBS_Transport_CloseConsumerRequest = 22,
  FBS_Transport_CloseDataProducerRequest = 23,
  FBS_Transport_CloseDataConsumerRequest = 24,
  FBS_Producer_EnableTraceEventRequest = 25,
  FBS_Consumer_SetPreferredLayersRequest = 26,
  FBS_Consumer_SetPriorityRequest = 27,
  FBS_Consumer_EnableTraceEventRequest = 28,
  FBS_DataConsumer_SetBufferedAmountLowThresholdRequest = 29,
  FBS_RtpObserver_AddProducerRequest = 30,
  FBS_RtpObserver_RemoveProducerRequest = 31
}

export function unionToBody(
  type: Body,
  accessor: (obj:AddProducerRequest|CloseConsumerRequest|CloseDataConsumerRequest|CloseDataProducerRequest|CloseProducerRequest|CloseRouterRequest|CloseRtpObserverRequest|CloseTransportRequest|CloseWebRtcServerRequest|ConsumeDataRequest|ConsumeRequest|CreateActiveSpeakerObserverRequest|CreateAudioLevelObserverRequest|CreateDirectTransportRequest|CreatePipeTransportRequest|CreatePlainTransportRequest|CreateRouterRequest|CreateWebRtcServerRequest|CreateWebRtcTransportRequest|EnableTraceEventRequest|FBS_Consumer_EnableTraceEventRequest|FBS_Producer_EnableTraceEventRequest|ProduceDataRequest|ProduceRequest|RemoveProducerRequest|SetBufferedAmountLowThresholdRequest|SetMaxIncomingBitrateRequest|SetMaxOutgoingBitrateRequest|SetPreferredLayersRequest|SetPriorityRequest|UpdateSettingsRequest) => AddProducerRequest|CloseConsumerRequest|CloseDataConsumerRequest|CloseDataProducerRequest|CloseProducerRequest|CloseRouterRequest|CloseRtpObserverRequest|CloseTransportRequest|CloseWebRtcServerRequest|ConsumeDataRequest|ConsumeRequest|CreateActiveSpeakerObserverRequest|CreateAudioLevelObserverRequest|CreateDirectTransportRequest|CreatePipeTransportRequest|CreatePlainTransportRequest|CreateRouterRequest|CreateWebRtcServerRequest|CreateWebRtcTransportRequest|EnableTraceEventRequest|FBS_Consumer_EnableTraceEventRequest|FBS_Producer_EnableTraceEventRequest|ProduceDataRequest|ProduceRequest|RemoveProducerRequest|SetBufferedAmountLowThresholdRequest|SetMaxIncomingBitrateRequest|SetMaxOutgoingBitrateRequest|SetPreferredLayersRequest|SetPriorityRequest|UpdateSettingsRequest|null
): AddProducerRequest|CloseConsumerRequest|CloseDataConsumerRequest|CloseDataProducerRequest|CloseProducerRequest|CloseRouterRequest|CloseRtpObserverRequest|CloseTransportRequest|CloseWebRtcServerRequest|ConsumeDataRequest|ConsumeRequest|CreateActiveSpeakerObserverRequest|CreateAudioLevelObserverRequest|CreateDirectTransportRequest|CreatePipeTransportRequest|CreatePlainTransportRequest|CreateRouterRequest|CreateWebRtcServerRequest|CreateWebRtcTransportRequest|EnableTraceEventRequest|FBS_Consumer_EnableTraceEventRequest|FBS_Producer_EnableTraceEventRequest|ProduceDataRequest|ProduceRequest|RemoveProducerRequest|SetBufferedAmountLowThresholdRequest|SetMaxIncomingBitrateRequest|SetMaxOutgoingBitrateRequest|SetPreferredLayersRequest|SetPriorityRequest|UpdateSettingsRequest|null {
  switch(Body[type]) {
    case 'NONE': return null; 
    case 'FBS_Worker_UpdateSettingsRequest': return accessor(new UpdateSettingsRequest())! as UpdateSettingsRequest;
    case 'FBS_Worker_CreateWebRtcServerRequest': return accessor(new CreateWebRtcServerRequest())! as CreateWebRtcServerRequest;
    case 'FBS_Worker_CloseWebRtcServerRequest': return accessor(new CloseWebRtcServerRequest())! as CloseWebRtcServerRequest;
    case 'FBS_Worker_CreateRouterRequest': return accessor(new CreateRouterRequest())! as CreateRouterRequest;
    case 'FBS_Worker_CloseRouterRequest': return accessor(new CloseRouterRequest())! as CloseRouterRequest;
    case 'FBS_Router_CreateWebRtcTransportRequest': return accessor(new CreateWebRtcTransportRequest())! as CreateWebRtcTransportRequest;
    case 'FBS_Router_CreatePlainTransportRequest': return accessor(new CreatePlainTransportRequest())! as CreatePlainTransportRequest;
    case 'FBS_Router_CreatePipeTransportRequest': return accessor(new CreatePipeTransportRequest())! as CreatePipeTransportRequest;
    case 'FBS_Router_CreateDirectTransportRequest': return accessor(new CreateDirectTransportRequest())! as CreateDirectTransportRequest;
    case 'FBS_Router_CreateActiveSpeakerObserverRequest': return accessor(new CreateActiveSpeakerObserverRequest())! as CreateActiveSpeakerObserverRequest;
    case 'FBS_Router_CreateAudioLevelObserverRequest': return accessor(new CreateAudioLevelObserverRequest())! as CreateAudioLevelObserverRequest;
    case 'FBS_Router_CloseTransportRequest': return accessor(new CloseTransportRequest())! as CloseTransportRequest;
    case 'FBS_Router_CloseRtpObserverRequest': return accessor(new CloseRtpObserverRequest())! as CloseRtpObserverRequest;
    case 'FBS_Transport_SetMaxIncomingBitrateRequest': return accessor(new SetMaxIncomingBitrateRequest())! as SetMaxIncomingBitrateRequest;
    case 'FBS_Transport_SetMaxOutgoingBitrateRequest': return accessor(new SetMaxOutgoingBitrateRequest())! as SetMaxOutgoingBitrateRequest;
    case 'FBS_Transport_ProduceRequest': return accessor(new ProduceRequest())! as ProduceRequest;
    case 'FBS_Transport_ConsumeRequest': return accessor(new ConsumeRequest())! as ConsumeRequest;
    case 'FBS_Transport_ProduceDataRequest': return accessor(new ProduceDataRequest())! as ProduceDataRequest;
    case 'FBS_Transport_ConsumeDataRequest': return accessor(new ConsumeDataRequest())! as ConsumeDataRequest;
    case 'FBS_Transport_EnableTraceEventRequest': return accessor(new EnableTraceEventRequest())! as EnableTraceEventRequest;
    case 'FBS_Transport_CloseProducerRequest': return accessor(new CloseProducerRequest())! as CloseProducerRequest;
    case 'FBS_Transport_CloseConsumerRequest': return accessor(new CloseConsumerRequest())! as CloseConsumerRequest;
    case 'FBS_Transport_CloseDataProducerRequest': return accessor(new CloseDataProducerRequest())! as CloseDataProducerRequest;
    case 'FBS_Transport_CloseDataConsumerRequest': return accessor(new CloseDataConsumerRequest())! as CloseDataConsumerRequest;
    case 'FBS_Producer_EnableTraceEventRequest': return accessor(new FBS_Producer_EnableTraceEventRequest())! as FBS_Producer_EnableTraceEventRequest;
    case 'FBS_Consumer_SetPreferredLayersRequest': return accessor(new SetPreferredLayersRequest())! as SetPreferredLayersRequest;
    case 'FBS_Consumer_SetPriorityRequest': return accessor(new SetPriorityRequest())! as SetPriorityRequest;
    case 'FBS_Consumer_EnableTraceEventRequest': return accessor(new FBS_Consumer_EnableTraceEventRequest())! as FBS_Consumer_EnableTraceEventRequest;
    case 'FBS_DataConsumer_SetBufferedAmountLowThresholdRequest': return accessor(new SetBufferedAmountLowThresholdRequest())! as SetBufferedAmountLowThresholdRequest;
    case 'FBS_RtpObserver_AddProducerRequest': return accessor(new AddProducerRequest())! as AddProducerRequest;
    case 'FBS_RtpObserver_RemoveProducerRequest': return accessor(new RemoveProducerRequest())! as RemoveProducerRequest;
    default: return null;
  }
}

export function unionListToBody(
  type: Body, 
  accessor: (index: number, obj:AddProducerRequest|CloseConsumerRequest|CloseDataConsumerRequest|CloseDataProducerRequest|CloseProducerRequest|CloseRouterRequest|CloseRtpObserverRequest|CloseTransportRequest|CloseWebRtcServerRequest|ConsumeDataRequest|ConsumeRequest|CreateActiveSpeakerObserverRequest|CreateAudioLevelObserverRequest|CreateDirectTransportRequest|CreatePipeTransportRequest|CreatePlainTransportRequest|CreateRouterRequest|CreateWebRtcServerRequest|CreateWebRtcTransportRequest|EnableTraceEventRequest|FBS_Consumer_EnableTraceEventRequest|FBS_Producer_EnableTraceEventRequest|ProduceDataRequest|ProduceRequest|RemoveProducerRequest|SetBufferedAmountLowThresholdRequest|SetMaxIncomingBitrateRequest|SetMaxOutgoingBitrateRequest|SetPreferredLayersRequest|SetPriorityRequest|UpdateSettingsRequest) => AddProducerRequest|CloseConsumerRequest|CloseDataConsumerRequest|CloseDataProducerRequest|CloseProducerRequest|CloseRouterRequest|CloseRtpObserverRequest|CloseTransportRequest|CloseWebRtcServerRequest|ConsumeDataRequest|ConsumeRequest|CreateActiveSpeakerObserverRequest|CreateAudioLevelObserverRequest|CreateDirectTransportRequest|CreatePipeTransportRequest|CreatePlainTransportRequest|CreateRouterRequest|CreateWebRtcServerRequest|CreateWebRtcTransportRequest|EnableTraceEventRequest|FBS_Consumer_EnableTraceEventRequest|FBS_Producer_EnableTraceEventRequest|ProduceDataRequest|ProduceRequest|RemoveProducerRequest|SetBufferedAmountLowThresholdRequest|SetMaxIncomingBitrateRequest|SetMaxOutgoingBitrateRequest|SetPreferredLayersRequest|SetPriorityRequest|UpdateSettingsRequest|null, 
  index: number
): AddProducerRequest|CloseConsumerRequest|CloseDataConsumerRequest|CloseDataProducerRequest|CloseProducerRequest|CloseRouterRequest|CloseRtpObserverRequest|CloseTransportRequest|CloseWebRtcServerRequest|ConsumeDataRequest|ConsumeRequest|CreateActiveSpeakerObserverRequest|CreateAudioLevelObserverRequest|CreateDirectTransportRequest|CreatePipeTransportRequest|CreatePlainTransportRequest|CreateRouterRequest|CreateWebRtcServerRequest|CreateWebRtcTransportRequest|EnableTraceEventRequest|FBS_Consumer_EnableTraceEventRequest|FBS_Producer_EnableTraceEventRequest|ProduceDataRequest|ProduceRequest|RemoveProducerRequest|SetBufferedAmountLowThresholdRequest|SetMaxIncomingBitrateRequest|SetMaxOutgoingBitrateRequest|SetPreferredLayersRequest|SetPriorityRequest|UpdateSettingsRequest|null {
  switch(Body[type]) {
    case 'NONE': return null; 
    case 'FBS_Worker_UpdateSettingsRequest': return accessor(index, new UpdateSettingsRequest())! as UpdateSettingsRequest;
    case 'FBS_Worker_CreateWebRtcServerRequest': return accessor(index, new CreateWebRtcServerRequest())! as CreateWebRtcServerRequest;
    case 'FBS_Worker_CloseWebRtcServerRequest': return accessor(index, new CloseWebRtcServerRequest())! as CloseWebRtcServerRequest;
    case 'FBS_Worker_CreateRouterRequest': return accessor(index, new CreateRouterRequest())! as CreateRouterRequest;
    case 'FBS_Worker_CloseRouterRequest': return accessor(index, new CloseRouterRequest())! as CloseRouterRequest;
    case 'FBS_Router_CreateWebRtcTransportRequest': return accessor(index, new CreateWebRtcTransportRequest())! as CreateWebRtcTransportRequest;
    case 'FBS_Router_CreatePlainTransportRequest': return accessor(index, new CreatePlainTransportRequest())! as CreatePlainTransportRequest;
    case 'FBS_Router_CreatePipeTransportRequest': return accessor(index, new CreatePipeTransportRequest())! as CreatePipeTransportRequest;
    case 'FBS_Router_CreateDirectTransportRequest': return accessor(index, new CreateDirectTransportRequest())! as CreateDirectTransportRequest;
    case 'FBS_Router_CreateActiveSpeakerObserverRequest': return accessor(index, new CreateActiveSpeakerObserverRequest())! as CreateActiveSpeakerObserverRequest;
    case 'FBS_Router_CreateAudioLevelObserverRequest': return accessor(index, new CreateAudioLevelObserverRequest())! as CreateAudioLevelObserverRequest;
    case 'FBS_Router_CloseTransportRequest': return accessor(index, new CloseTransportRequest())! as CloseTransportRequest;
    case 'FBS_Router_CloseRtpObserverRequest': return accessor(index, new CloseRtpObserverRequest())! as CloseRtpObserverRequest;
    case 'FBS_Transport_SetMaxIncomingBitrateRequest': return accessor(index, new SetMaxIncomingBitrateRequest())! as SetMaxIncomingBitrateRequest;
    case 'FBS_Transport_SetMaxOutgoingBitrateRequest': return accessor(index, new SetMaxOutgoingBitrateRequest())! as SetMaxOutgoingBitrateRequest;
    case 'FBS_Transport_ProduceRequest': return accessor(index, new ProduceRequest())! as ProduceRequest;
    case 'FBS_Transport_ConsumeRequest': return accessor(index, new ConsumeRequest())! as ConsumeRequest;
    case 'FBS_Transport_ProduceDataRequest': return accessor(index, new ProduceDataRequest())! as ProduceDataRequest;
    case 'FBS_Transport_ConsumeDataRequest': return accessor(index, new ConsumeDataRequest())! as ConsumeDataRequest;
    case 'FBS_Transport_EnableTraceEventRequest': return accessor(index, new EnableTraceEventRequest())! as EnableTraceEventRequest;
    case 'FBS_Transport_CloseProducerRequest': return accessor(index, new CloseProducerRequest())! as CloseProducerRequest;
    case 'FBS_Transport_CloseConsumerRequest': return accessor(index, new CloseConsumerRequest())! as CloseConsumerRequest;
    case 'FBS_Transport_CloseDataProducerRequest': return accessor(index, new CloseDataProducerRequest())! as CloseDataProducerRequest;
    case 'FBS_Transport_CloseDataConsumerRequest': return accessor(index, new CloseDataConsumerRequest())! as CloseDataConsumerRequest;
    case 'FBS_Producer_EnableTraceEventRequest': return accessor(index, new FBS_Producer_EnableTraceEventRequest())! as FBS_Producer_EnableTraceEventRequest;
    case 'FBS_Consumer_SetPreferredLayersRequest': return accessor(index, new SetPreferredLayersRequest())! as SetPreferredLayersRequest;
    case 'FBS_Consumer_SetPriorityRequest': return accessor(index, new SetPriorityRequest())! as SetPriorityRequest;
    case 'FBS_Consumer_EnableTraceEventRequest': return accessor(index, new FBS_Consumer_EnableTraceEventRequest())! as FBS_Consumer_EnableTraceEventRequest;
    case 'FBS_DataConsumer_SetBufferedAmountLowThresholdRequest': return accessor(index, new SetBufferedAmountLowThresholdRequest())! as SetBufferedAmountLowThresholdRequest;
    case 'FBS_RtpObserver_AddProducerRequest': return accessor(index, new AddProducerRequest())! as AddProducerRequest;
    case 'FBS_RtpObserver_RemoveProducerRequest': return accessor(index, new RemoveProducerRequest())! as RemoveProducerRequest;
    default: return null;
  }
}
