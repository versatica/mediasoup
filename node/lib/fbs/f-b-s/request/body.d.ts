import { ConsumeRequest } from '../../f-b-s/transport/consume-request';
export declare enum Body {
    NONE = 0,
    FBS_Transport_ConsumeRequest = 1
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeRequest) => ConsumeRequest | null): ConsumeRequest | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeRequest) => ConsumeRequest | null, index: number): ConsumeRequest | null;
//# sourceMappingURL=body.d.ts.map