import { WebRtcTransportDump } from '../../fbs/transport/web-rtc-transport-dump';
export declare enum SpecificTransportDumpData {
    NONE = 0,
    WebRtcTransportDump = 1
}
export declare function unionToSpecificTransportDumpData(type: SpecificTransportDumpData, accessor: (obj: WebRtcTransportDump) => WebRtcTransportDump | null): WebRtcTransportDump | null;
export declare function unionListToSpecificTransportDumpData(type: SpecificTransportDumpData, accessor: (index: number, obj: WebRtcTransportDump) => WebRtcTransportDump | null, index: number): WebRtcTransportDump | null;
//# sourceMappingURL=specific-transport-dump-data.d.ts.map