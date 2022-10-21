import { WebRtcTransportDump } from '../../fbs/transport/web-rtc-transport-dump';
export declare enum TransportDumpData {
    NONE = 0,
    WebRtcTransportDump = 1
}
export declare function unionToTransportDumpData(type: TransportDumpData, accessor: (obj: WebRtcTransportDump) => WebRtcTransportDump | null): WebRtcTransportDump | null;
export declare function unionListToTransportDumpData(type: TransportDumpData, accessor: (index: number, obj: WebRtcTransportDump) => WebRtcTransportDump | null, index: number): WebRtcTransportDump | null;
//# sourceMappingURL=transport-dump-data.d.ts.map