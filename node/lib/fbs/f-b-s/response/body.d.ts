import { ConsumeResponse } from '../../f-b-s/transport/consume-response';
import { Dump } from '../../f-b-s/worker/dump';
import { ResourceUsage } from '../../f-b-s/worker/resource-usage';
export declare enum Body {
    NONE = 0,
    FBS_Worker_Dump = 1,
    FBS_Worker_ResourceUsage = 2,
    FBS_Transport_ConsumeResponse = 3
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeResponse | Dump | ResourceUsage) => ConsumeResponse | Dump | ResourceUsage | null): ConsumeResponse | Dump | ResourceUsage | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeResponse | Dump | ResourceUsage) => ConsumeResponse | Dump | ResourceUsage | null, index: number): ConsumeResponse | Dump | ResourceUsage | null;
//# sourceMappingURL=body.d.ts.map