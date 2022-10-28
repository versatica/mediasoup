import * as flatbuffers from 'flatbuffers';
import { BaseTransportDumpT } from '../../fbs/transport/base-transport-dump';
import { DirectTransportDumpT } from '../../fbs/transport/direct-transport-dump';
import { PipeTransportDumpT } from '../../fbs/transport/pipe-transport-dump';
import { PlainTransportDumpT } from '../../fbs/transport/plain-transport-dump';
import { TransportDumpData } from '../../fbs/transport/transport-dump-data';
import { WebRtcTransportDumpT } from '../../fbs/transport/web-rtc-transport-dump';
export declare class TransportDumpResponse {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): TransportDumpResponse;
    static getRootAsTransportDumpResponse(bb: flatbuffers.ByteBuffer, obj?: TransportDumpResponse): TransportDumpResponse;
    static getSizePrefixedRootAsTransportDumpResponse(bb: flatbuffers.ByteBuffer, obj?: TransportDumpResponse): TransportDumpResponse;
    dataType(): TransportDumpData;
    data<T extends flatbuffers.Table>(obj: any): any | null;
    static startTransportDumpResponse(builder: flatbuffers.Builder): void;
    static addDataType(builder: flatbuffers.Builder, dataType: TransportDumpData): void;
    static addData(builder: flatbuffers.Builder, dataOffset: flatbuffers.Offset): void;
    static endTransportDumpResponse(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createTransportDumpResponse(builder: flatbuffers.Builder, dataType: TransportDumpData, dataOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): TransportDumpResponseT;
    unpackTo(_o: TransportDumpResponseT): void;
}
export declare class TransportDumpResponseT {
    dataType: TransportDumpData;
    data: BaseTransportDumpT | DirectTransportDumpT | PipeTransportDumpT | PlainTransportDumpT | WebRtcTransportDumpT | null;
    constructor(dataType?: TransportDumpData, data?: BaseTransportDumpT | DirectTransportDumpT | PipeTransportDumpT | PlainTransportDumpT | WebRtcTransportDumpT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=transport-dump-response.d.ts.map