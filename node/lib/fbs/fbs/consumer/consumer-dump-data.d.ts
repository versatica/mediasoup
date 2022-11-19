import { BaseConsumerDump } from '../../fbs/consumer/base-consumer-dump';
import { PipeConsumerDump } from '../../fbs/consumer/pipe-consumer-dump';
import { SimpleConsumerDump } from '../../fbs/consumer/simple-consumer-dump';
import { SimulcastConsumerDump } from '../../fbs/consumer/simulcast-consumer-dump';
import { SvcConsumerDump } from '../../fbs/consumer/svc-consumer-dump';
export declare enum ConsumerDumpData {
    NONE = 0,
    BaseConsumerDump = 1,
    SimpleConsumerDump = 2,
    SimulcastConsumerDump = 3,
    SvcConsumerDump = 4,
    PipeConsumerDump = 5
}
export declare function unionToConsumerDumpData(type: ConsumerDumpData, accessor: (obj: BaseConsumerDump | PipeConsumerDump | SimpleConsumerDump | SimulcastConsumerDump | SvcConsumerDump) => BaseConsumerDump | PipeConsumerDump | SimpleConsumerDump | SimulcastConsumerDump | SvcConsumerDump | null): BaseConsumerDump | PipeConsumerDump | SimpleConsumerDump | SimulcastConsumerDump | SvcConsumerDump | null;
export declare function unionListToConsumerDumpData(type: ConsumerDumpData, accessor: (index: number, obj: BaseConsumerDump | PipeConsumerDump | SimpleConsumerDump | SimulcastConsumerDump | SvcConsumerDump) => BaseConsumerDump | PipeConsumerDump | SimpleConsumerDump | SimulcastConsumerDump | SvcConsumerDump | null, index: number): BaseConsumerDump | PipeConsumerDump | SimpleConsumerDump | SimulcastConsumerDump | SvcConsumerDump | null;
//# sourceMappingURL=consumer-dump-data.d.ts.map