import * as flatbuffers from 'flatbuffers';
import { BaseTransportDumpT } from '../../fbs/transport/base-transport-dump';
import { DirectTransportDumpT } from '../../fbs/transport/direct-transport-dump';
import { PipeTransportDumpT } from '../../fbs/transport/pipe-transport-dump';
import { PlainTransportDumpT } from '../../fbs/transport/plain-transport-dump';
import { TransportDumpData } from '../../fbs/transport/transport-dump-data';
import { WebRtcTransportDumpT } from '../../fbs/transport/web-rtc-transport-dump';
export declare class DumpResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): DumpResponse;
    static getRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    static getSizePrefixedRootAsDumpResponse(bb: flatbuffers.ByteBuffer, obj?: DumpResponse): DumpResponse;
    dataType(): TransportDumpData;
    data<T extends flatbuffers.Table>(obj: any): any | null;
    static startDumpResponse(builder: flatbuffers.Builder): void;
    static addDataType(builder: flatbuffers.Builder, dataType: TransportDumpData): void;
    static addData(builder: flatbuffers.Builder, dataOffset: flatbuffers.Offset): void;
    static endDumpResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createDumpResponse(builder: flatbuffers.Builder, dataType: TransportDumpData, dataOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): DumpResponseT;
    unpackTo(_o: DumpResponseT): void;
}
export declare class DumpResponseT {
    dataType: TransportDumpData;
    data: BaseTransportDumpT | DirectTransportDumpT | PipeTransportDumpT | PlainTransportDumpT | WebRtcTransportDumpT | null;
    constructor(dataType?: TransportDumpData, data?: BaseTransportDumpT | DirectTransportDumpT | PipeTransportDumpT | PlainTransportDumpT | WebRtcTransportDumpT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=dump-response.d.ts.map