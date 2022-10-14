import { ConsumeRequest } from '../../f-b-s/transport/consume-request';
import { DumpRequest } from '../../f-b-s/worker/dump-request';
export declare enum Body {
    NONE = 0,
    FBS_Worker_DumpRequest = 1,
    FBS_Transport_ConsumeRequest = 2
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeRequest | DumpRequest) => ConsumeRequest | DumpRequest | null): ConsumeRequest | DumpRequest | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeRequest | DumpRequest) => ConsumeRequest | DumpRequest | null, index: number): ConsumeRequest | DumpRequest | null;
//# sourceMappingURL=body.d.ts.map