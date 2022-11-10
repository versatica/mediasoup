import { CreatePipeTransportRequest } from '../../fbs/router/create-pipe-transport-request';
import { CreatePlainTransportRequest } from '../../fbs/router/create-plain-transport-request';
import { CreateWebRtcTransportRequest } from '../../fbs/router/create-web-rtc-transport-request';
import { ConsumeRequest } from '../../fbs/transport/consume-request';
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
    FBS_Transport_ConsumeRequest = 9
}
export declare function unionToBody(type: Body, accessor: (obj: CloseRouterRequest | CloseWebRtcServerRequest | ConsumeRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | UpdateSettingsRequest) => CloseRouterRequest | CloseWebRtcServerRequest | ConsumeRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | UpdateSettingsRequest | null): CloseRouterRequest | CloseWebRtcServerRequest | ConsumeRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | UpdateSettingsRequest | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: CloseRouterRequest | CloseWebRtcServerRequest | ConsumeRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | UpdateSettingsRequest) => CloseRouterRequest | CloseWebRtcServerRequest | ConsumeRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | UpdateSettingsRequest | null, index: number): CloseRouterRequest | CloseWebRtcServerRequest | ConsumeRequest | CreatePipeTransportRequest | CreatePlainTransportRequest | CreateRouterRequest | CreateWebRtcServerRequest | CreateWebRtcTransportRequest | UpdateSettingsRequest | null;
//# sourceMappingURL=body.d.ts.map