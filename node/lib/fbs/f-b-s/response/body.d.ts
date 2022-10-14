import { ConsumeResponse } from '../../f-b-s/transport/consume-response';
import { DumpResponse } from '../../f-b-s/worker/dump-response';
export declare enum Body {
    NONE = 0,
    FBS_Worker_DumpResponse = 1,
    FBS_Transport_ConsumeResponse = 2
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeResponse | DumpResponse) => ConsumeResponse | DumpResponse | null): ConsumeResponse | DumpResponse | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeResponse | DumpResponse) => ConsumeResponse | DumpResponse | null, index: number): ConsumeResponse | DumpResponse | null;
//# sourceMappingURL=body.d.ts.map