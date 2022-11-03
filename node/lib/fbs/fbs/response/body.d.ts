import { DumpResponse as FBS_Router_DumpResponse } from '../../fbs/router/dump-response';
import { ConsumeResponse } from '../../fbs/transport/consume-response';
import { DumpResponse as FBS_Transport_DumpResponse } from '../../fbs/transport/dump-response';
import { DumpResponse as FBS_WebRtcServer_DumpResponse } from '../../fbs/web-rtc-server/dump-response';
import { DumpResponse } from '../../fbs/worker/dump-response';
import { ResourceUsageResponse } from '../../fbs/worker/resource-usage-response';
export declare enum Body {
    NONE = 0,
    FBS_Worker_DumpResponse = 1,
    FBS_Worker_ResourceUsageResponse = 2,
    FBS_WebRtcServer_DumpResponse = 3,
    FBS_Router_DumpResponse = 4,
    FBS_Transport_DumpResponse = 5,
    FBS_Transport_ConsumeResponse = 6
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeResponse | DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | ResourceUsageResponse) => ConsumeResponse | DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | ResourceUsageResponse | null): ConsumeResponse | DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | ResourceUsageResponse | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeResponse | DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | ResourceUsageResponse) => ConsumeResponse | DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | ResourceUsageResponse | null, index: number): ConsumeResponse | DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | ResourceUsageResponse | null;
//# sourceMappingURL=body.d.ts.map