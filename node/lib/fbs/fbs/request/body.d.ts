import { CloseRtpObserverRequest } from '../../fbs/router/close-rtp-observer-request';
import { CloseTransportRequest } from '../../fbs/router/close-transport-request';
import { CreateActiveSpeakerObserverRequest } from '../../fbs/router/create-active-speaker-observer-request';
import { CreateAudioLevelObserverRequest } from '../../fbs/router/create-audio-level-observer-request';
import { CreateDirectTransportRequest } from '../../fbs/router/create-direct-transport-request';
import { CreatePipeTransportRequest } from '../../fbs/router/create-pipe-transport-request';
import { CreatePlainTransportRequest } from '../../fbs/router/create-plain-transport-request';
import { CreateWebRtcTransportRequest } from '../../fbs/router/create-web-rtc-transport-request';
import { ConsumeRequest } from '../../fbs/transport/consume-request';
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
    FBS_Transport_ConsumeRequest = 16
}
export declare function unionToBody(type: Body, accessor: (obj: CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | UpdateSettingsRequest) => CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | UpdateSettingsRequest | null): CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | UpdateSettingsRequest | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | UpdateSettingsRequest) => CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | UpdateSettingsRequest | null, index: number): CloseRouterRequest | CloseRtpObserverRequest | CloseTransportRequest | CloseWebRtcServerRequest | ConsumeRequest | CreateActiveSpeakerObserverRequest | CreateAudioLevelObserverRequest | CreateDirectTransportRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | SetMaxIncomingBitrateRequest | SetMaxOutgoingBitrateRequest | UpdateSettingsRequest | null;
//# sourceMappingURL=body.d.ts.map