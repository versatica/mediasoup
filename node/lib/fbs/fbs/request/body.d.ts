import { ConsumeRequest } from '../../fbs/transport/consume-request';
import { CreateWebRtcServerRequest } from '../../fbs/worker/create-web-rtc-server-request';
import { UpdateableSettings } from '../../fbs/worker/updateable-settings';
export declare enum Body {
    NONE = 0,
    FBS_Worker_UpdateableSettings = 1,
    FBS_Worker_CreateWebRtcServerRequest = 2,
    FBS_Transport_ConsumeRequest = 3
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings) => ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings | null): ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings) => ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings | null, index: number): ConsumeRequest | CreateWebRtcServerRequest | UpdateableSettings | null;
//# sourceMappingURL=body.d.ts.map