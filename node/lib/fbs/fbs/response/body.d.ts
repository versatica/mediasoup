import { ConsumeResponse } from '../../fbs/transport/consume-response';
import { WebRtcServerDump } from '../../fbs/web-rtc-server/web-rtc-server-dump';
import { ResourceUsage } from '../../fbs/worker/resource-usage';
import { WorkerDump } from '../../fbs/worker/worker-dump';
export declare enum Body {
    NONE = 0,
    FBS_Worker_WorkerDump = 1,
    FBS_Worker_ResourceUsage = 2,
    FBS_WebRtcServer_WebRtcServerDump = 3,
    FBS_Transport_ConsumeResponse = 4
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeResponse | ResourceUsage | WebRtcServerDump | WorkerDump) => ConsumeResponse | ResourceUsage | WebRtcServerDump | WorkerDump | null): ConsumeResponse | ResourceUsage | WebRtcServerDump | WorkerDump | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeResponse | ResourceUsage | WebRtcServerDump | WorkerDump) => ConsumeResponse | ResourceUsage | WebRtcServerDump | WorkerDump | null, index: number): ConsumeResponse | ResourceUsage | WebRtcServerDump | WorkerDump | null;
//# sourceMappingURL=body.d.ts.map