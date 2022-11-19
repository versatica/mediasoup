import * as flatbuffers from 'flatbuffers';
import { BaseConsumerDumpT } from '../../fbs/consumer/base-consumer-dump';
import { ConsumerDumpData } from '../../fbs/consumer/consumer-dump-data';
import { PipeConsumerDumpT } from '../../fbs/consumer/pipe-consumer-dump';
import { SimpleConsumerDumpT } from '../../fbs/consumer/simple-consumer-dump';
import { SimulcastConsumerDumpT } from '../../fbs/consumer/simulcast-consumer-dump';
import { SvcConsumerDumpT } from '../../fbs/consumer/svc-consumer-dump';
export declare class DumpResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DumpResponse;
    static getRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    static getSizePrefixedRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    dataType(): ConsumerDumpData;
    data<T extends flatbuffers.Table>(obj: any): any | null;
    static startDumpResponse(builder: flatbuffers.Builder): void;
    static addDataType(builder: flatbuffers.Builder, dataType: ConsumerDumpData): void;
    static addData(builder: flatbuffers.Builder, dataOffset: flatbuffers.Offset): void;
    static endDumpResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDumpResponse(builder: flatbuffers.Builder, dataType: ConsumerDumpData, dataOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): DumpResponseT;
    unpackTo(_o: DumpResponseT): void;
}
export declare class DumpResponseT {
    dataType: ConsumerDumpData;
    data: BaseConsumerDumpT | PipeConsumerDumpT | SimpleConsumerDumpT | SimulcastConsumerDumpT | SvcConsumerDumpT | null;
    constructor(dataType?: ConsumerDumpData, data?: BaseConsumerDumpT | PipeConsumerDumpT | SimpleConsumerDumpT | SimulcastConsumerDumpT | SvcConsumerDumpT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump-response.d.ts.map