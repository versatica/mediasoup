import { ConsumeRequest } from '../../f-b-s/transport/consume-request';
import { UpdateableSettings } from '../../f-b-s/worker/updateable-settings';
export declare enum Body {
    NONE = 0,
    FBS_Transport_ConsumeRequest = 1,
    FBS_Worker_UpdateableSettings = 2
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeRequest | UpdateableSettings) => ConsumeRequest | UpdateableSettings | null): ConsumeRequest | UpdateableSettings | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeRequest | UpdateableSettings) => ConsumeRequest | UpdateableSettings | null, index: number): ConsumeRequest | UpdateableSettings | null;
//# sourceMappingURL=body.d.ts.map