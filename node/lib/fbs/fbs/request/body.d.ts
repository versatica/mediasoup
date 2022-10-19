import { ConsumeRequest } from '../../fbs/transport/consume-request';
import { CloseWebRtcServerRequest } from '../../fbs/worker/close-web-rtc-server-request';
import { CreateWebRtcServerRequest } from '../../fbs/worker/create-web-rtc-server-request';
import { UpdateableSettings } from '../../fbs/worker/updateable-settings';
export declare enum Body {
    NONE = 0,
    FBS_Worker_UpdateableSettings = 1,
    FBS_Worker_CreateWebRtcServerRequest = 2,
    FBS_Worker_CloseWebRtcServerRequest = 3,
    FBS_Transport_ConsumeRequest = 4
}
export declare function unionToBody(type: Body, accessor: (obj: CloseWebRtcServerRequest | ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings) => CloseWebRtcServerRequest | ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings | null): CloseWebRtcServerRequest | ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: CloseWebRtcServerRequest | ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings) => CloseWebRtcServerRequest | ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings | null, index: number): CloseWebRtcServerRequest | ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings | null;
//# sourceMappingURL=body.d.ts.map