import * as flatbuffers from 'flatbuffers';
import { SpecificTransportDumpData } from '../../fbs/transport/specific-transport-dump-data';
import { WebRtcTransportDumpT } from '../../fbs/transport/web-rtc-transport-dump';
export declare class SpecificTransportDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): SpecificTransportDump;
    static getRootAsSpecificTransportDump(bb: flatbuffers.ByteBuffer, obj?: SpecificTransportDump): SpecificTransportDump;
    static getSizePrefixedRootAsSpecificTransportDump(bb: flatbuffers.ByteBuffer, obj?: SpecificTransportDump): SpecificTransportDump;
    dataType(): SpecificTransportDumpData;
    data<T extends flatbuffers.Table>(obj: any): any | null;
    static startSpecificTransportDump(builder: flatbuffers.Builder): void;
    static addDataType(builder: flatbuffers.Builder, dataType: SpecificTransportDumpData): void;
    static addData(builder: flatbuffers.Builder, dataOffset: flatbuffers.Offset): void;
    static endSpecificTransportDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    static createSpecificTransportDump(builder: flatbuffers.Builder, dataType: SpecificTransportDumpData, dataOffset: flatbuffers.Offset): flatbuffers.Offset;
    unpack(): SpecificTransportDumpT;
    unpackTo(_o: SpecificTransportDumpT): void;
}
export declare class SpecificTransportDumpT {
    dataType: SpecificTransportDumpData;
    data: WebRtcTransportDumpT | null;
    constructor(dataType?: SpecificTransportDumpData, data?: WebRtcTransportDumpT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=specific-transport-dump.d.ts.map