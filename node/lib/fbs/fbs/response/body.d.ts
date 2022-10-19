import { ConsumeResponse } from '../../fbs/transport/consume-response';
import { WebRtcServerDump } from '../../fbs/web-rtc-server/web-rtc-server-dump';
import { Dump } from '../../fbs/worker/dump';
import { ResourceUsage } from '../../fbs/worker/resource-usage';
export declare enum Body {
    NONE = 0,
    FBS_Worker_Dump = 1,
    FBS_Worker_ResourceUsage = 2,
    FBS_WebRtcServer_WebRtcServerDump = 3,
    FBS_Transport_ConsumeResponse = 4
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeResponse | Dump | ResourceUsage | WebRtcServerDump) => ConsumeResponse | Dump | ResourceUsage | WebRtcServerDump | null): ConsumeResponse | Dump | ResourceUsage | WebRtcServerDump | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeResponse | Dump | ResourceUsage | WebRtcServerDump) => ConsumeResponse | Dump | ResourceUsage | WebRtcServerDump | null, index: number): ConsumeResponse | Dump | ResourceUsage | WebRtcServerDump | null;
//# sourceMappingURL=body.d.ts.map