import { BaseTransportDump } from '../../fbs/transport/base-transport-dump';
import { WebRtcTransportDump } from '../../fbs/transport/web-rtc-transport-dump';
export declare enum TransportDumpData {
    NONE = 0,
    BaseTransportDump = 1,
    WebRtcTransportDump = 2
}
export declare function unionToTransportDumpData(type: TransportDumpData, accessor: (obj: BaseTransportDump | WebRtcTransportDump) => BaseTransportDump | WebRtcTransportDump | null): BaseTransportDump | WebRtcTransportDump | null;
export declare function unionListToTransportDumpData(type: TransportDumpData, accessor: (index: number, obj: BaseTransportDump | WebRtcTransportDump) => BaseTransportDump | WebRtcTransportDump | null, index: number): BaseTransportDump | WebRtcTransportDump | null;
//# sourceMappingURL=transport-dump-data.d.ts.map