import { SetPreferredLayersResponse } from '../../fbs/consumer/set-preferred-layers-response';
import { SetPriorityResponse } from '../../fbs/consumer/set-priority-response';
import { DumpResponse as FBS_DataConsumer_DumpResponse } from '../../fbs/data-consumer/dump-response';
import { GetBufferedAmountResponse } from '../../fbs/data-consumer/get-buffered-amount-response';
import { DumpResponse as FBS_DataProducer_DumpResponse } from '../../fbs/data-producer/dump-response';
import { DumpResponse as FBS_Router_DumpResponse } from '../../fbs/router/dump-response';
import { ConsumeResponse } from '../../fbs/transport/consume-response';
import { DumpResponse as FBS_Transport_DumpResponse } from '../../fbs/transport/dump-response';
import { ProduceResponse } from '../../fbs/transport/produce-response';
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
    FBS_Transport_ProduceResponse = 6,
    FBS_Transport_ConsumeResponse = 7,
    FBS_Consumer_SetPreferredLayersResponse = 8,
    FBS_Consumer_SetPriorityResponse = 9,
    FBS_DataProducer_DumpResponse = 10,
    FBS_DataConsumer_GetBufferedAmountResponse = 11,
    FBS_DataConsumer_DumpResponse = 12
}
export declare function unionToBody(type: Body, accessor: (obj: ConsumeResponse | DumpResponse | FBS_DataConsumer_DumpResponse | FBS_DataProducer_DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | GetBufferedAmountResponse | ProduceResponse | ResourceUsageResponse | SetPreferredLayersResponse | SetPriorityResponse) => ConsumeResponse | DumpResponse | FBS_DataConsumer_DumpResponse | FBS_DataProducer_DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | GetBufferedAmountResponse | ProduceResponse | ResourceUsageResponse | SetPreferredLayersResponse | SetPriorityResponse | null): ConsumeResponse | DumpResponse | FBS_DataConsumer_DumpResponse | FBS_DataProducer_DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | GetBufferedAmountResponse | ProduceResponse | ResourceUsageResponse | SetPreferredLayersResponse | SetPriorityResponse | null;
export declare function unionListToBody(type: Body, accessor: (index: number, obj: ConsumeResponse | DumpResponse | FBS_DataConsumer_DumpResponse | FBS_DataProducer_DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | GetBufferedAmountResponse | ProduceResponse | ResourceUsageResponse | SetPreferredLayersResponse | SetPriorityResponse) => ConsumeResponse | DumpResponse | FBS_DataConsumer_DumpResponse | FBS_DataProducer_DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | GetBufferedAmountResponse | ProduceResponse | ResourceUsageResponse | SetPreferredLayersResponse | SetPriorityResponse | null, index: number): ConsumeResponse | DumpResponse | FBS_DataConsumer_DumpResponse | FBS_DataProducer_DumpResponse | FBS_Router_DumpResponse | FBS_Transport_DumpResponse | FBS_WebRtcServer_DumpResponse | GetBufferedAmountResponse | ProduceResponse | ResourceUsageResponse | SetPreferredLayersResponse | SetPriorityResponse | null;
//# sourceMappingURL=body.d.ts.map