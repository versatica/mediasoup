import { ConsumeResponse } from '../../f-b-s/transport/consume-response';
import { DumpResponse } from '../../f-b-s/worker/dump-response';
import { ResourceUsage } from '../../f-b-s/worker/resource-usage';
export declare enum Body {
    NONE = 0,
    FBS_Worker_DumpResponse = 1,
    FBS_Worker_ResourceUsage = 2,
    FBS_Transport_ConsumeResponse = 3
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeResponse | DumpResponse | ResourceUsage) => ConsumeResponse | DumpResponse | ResourceUsage | null): ConsumeResponse | DumpResponse | ResourceUsage | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeResponse | DumpResponse | ResourceUsage) => ConsumeResponse | DumpResponse | ResourceUsage | null, index: number): ConsumeResponse | DumpResponse | ResourceUsage | null;
//# sourceMappingURL=body.d.ts.map