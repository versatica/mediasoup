import * as flatbuffers from 'flatbuffers';
import { BaseTransportDumpT } from '../../fbs/transport/base-transport-dump';
import { TransportDumpData } from '../../fbs/transport/transport-dump-data';
import { WebRtcTransportDumpT } from '../../fbs/transport/web-rtc-transport-dump';
export declare class TransportDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): TransportDump;
    static getRootAsTransportDump(bb: flatbuffers.ByteBuffer, obj?: TransportDump): TransportDump;
    static getSizePrefixedRootAsTransportDump(bb: flatbuffers.ByteBuffer, obj?: TransportDump): TransportDump;
    dataType(): TransportDumpData;
    data<T extends flatbuffers.Table>(obj: any): any | null;
    static startTransportDump(builder: flatbuffers.Builder): void;
    static addDataType(builder: flatbuffers.Builder, dataType: TransportDumpData): void;
    static addData(builder: flatbuffers.Builder, dataOffset: flatbuffers.Offset): void;
    static endTransportDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createTransportDump(builder: flatbuffers.Builder, dataType: TransportDumpData, dataOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): TransportDumpT;
    unpackTo(_o: TransportDumpT): void;
}
export declare class TransportDumpT {
    dataType: TransportDumpData;
    data: BaseTransportDumpT | WebRtcTransportDumpT | null;
    constructor(dataType?: TransportDumpData, data?: BaseTransportDumpT | WebRtcTransportDumpT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=transport-dump.d.ts.map