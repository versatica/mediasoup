import * as flatbuffers from 'flatbuffers';
import { SrtpParameters, SrtpParametersT } from '../../fbs/transport/srtp-parameters';
import { TransportDumpResponse, TransportDumpResponseT } from '../../fbs/transport/transport-dump-response';
import { Tuple, TupleT } from '../../fbs/transport/tuple';
export declare class PlainTransportDump {
    bb: flatbuffers.ByteBuffer | null;
    bb_pos: number;
    __init(i: number, bb: flatbuffers.ByteBuffer): PlainTransportDump;
    static getRootAsPlainTransportDump(bb: flatbuffers.ByteBuffer, obj?: PlainTransportDump): PlainTransportDump;
    static getSizePrefixedRootAsPlainTransportDump(bb: flatbuffers.ByteBuffer, obj?: PlainTransportDump): PlainTransportDump;
    base(obj?: TransportDumpResponse): TransportDumpResponse | null;
    rtcMux(): boolean;
    comedia(): boolean;
    tuple(obj?: Tuple): Tuple | null;
    rtcpTuple(obj?: Tuple): Tuple | null;
    srtpParameters(obj?: SrtpParameters): SrtpParameters | null;
    static startPlainTransportDump(builder: flatbuffers.Builder): void;
    static addBase(builder: flatbuffers.Builder, baseOffset: flatbuffers.Offset): void;
    static addRtcMux(builder: flatbuffers.Builder, rtcMux: boolean): void;
    static addComedia(builder: flatbuffers.Builder, comedia: boolean): void;
    static addTuple(builder: flatbuffers.Builder, tupleOffset: flatbuffers.Offset): void;
    static addRtcpTuple(builder: flatbuffers.Builder, rtcpTupleOffset: flatbuffers.Offset): void;
    static addSrtpParameters(builder: flatbuffers.Builder, srtpParametersOffset: flatbuffers.Offset): void;
    static endPlainTransportDump(builder: flatbuffers.Builder): flatbuffers.Offset;
    unpack(): PlainTransportDumpT;
    unpackTo(_o: PlainTransportDumpT): void;
}
export declare class PlainTransportDumpT {
    base: TransportDumpResponseT | null;
    rtcMux: boolean;
    comedia: boolean;
    tuple: TupleT | null;
    rtcpTuple: TupleT | null;
    srtpParameters: SrtpParametersT | null;
    constructor(base?: TransportDumpResponseT | null, rtcMux?: boolean, comedia?: boolean, tuple?: TupleT | null, rtcpTuple?: TupleT | null, srtpParameters?: SrtpParametersT | null);
    pack(builder: flatbuffers.Builder): flatbuffers.Offset;
}
//# sourceMappingURL=plain-transport-dump.d.ts.map