import { RouterDumpResponse } from '../../fbs/router/router-dump-response';
import { ConsumeResponse } from '../../fbs/transport/consume-response';
import { TransportDumpResponse } from '../../fbs/transport/transport-dump-response';
import { WebRtcServerDumpResponse } from '../../fbs/web-rtc-server/web-rtc-server-dump-response';
import { ResourceUsageResponse } from '../../fbs/worker/resource-usage-response';
import { WorkerDumpResponse } from '../../fbs/worker/worker-dump-response';
export declare enum Body {
    NONE = 0,
    FBS_Worker_WorkerDumpResponse = 1,
    FBS_Worker_ResourceUsageResponse = 2,
    FBS_WebRtcServer_WebRtcServerDumpResponse = 3,
    FBS_Router_RouterDumpResponse = 4,
    FBS_Transport_TransportDumpResponse = 5,
    FBS_Transport_ConsumeResponse = 6
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeResponse | ResourceUsageResponse | RouterDumpResponse | TransportDumpResponse | WebRtcServerDumpResponse | WorkerDumpResponse) => ConsumeResponse | ResourceUsageResponse | RouterDumpResponse | TransportDumpResponse | WebRtcServerDumpResponse | WorkerDumpResponse | null): ConsumeResponse | ResourceUsageResponse | RouterDumpResponse | TransportDumpResponse | WebRtcServerDumpResponse | WorkerDumpResponse | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeResponse | ResourceUsageResponse | RouterDumpResponse | TransportDumpResponse | WebRtcServerDumpResponse | WorkerDumpResponse) => ConsumeResponse | ResourceUsageResponse | RouterDumpResponse | TransportDumpResponse | WebRtcServerDumpResponse | WorkerDumpResponse | null, index: number): ConsumeResponse | ResourceUsageResponse | RouterDumpResponse | TransportDumpResponse | WebRtcServerDumpResponse | WorkerDumpResponse | null;
//# sourceMappingURL=body.d.ts.map