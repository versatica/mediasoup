import { BaseTransportDump } from '../../fbs/transport/base-transport-dump';
import { DirectTransportDump } from '../../fbs/transport/direct-transport-dump';
import { PipeTransportDump } from '../../fbs/transport/pipe-transport-dump';
import { PlainTransportDump } from '../../fbs/transport/plain-transport-dump';
import { WebRtcTransportDump } from '../../fbs/transport/web-rtc-transport-dump';
export declare enum TransportDumpData {
    NONE = 0,
    BaseTransportDump = 1,
    DirectTransportDump = 2,
    PipeTransportDump = 3,
    PlainTransportDump = 4,
    WebRtcTransportDump = 5
}
export declare function unionToTransportDumpData(type: TransportDumpData, accessor: (obj: BaseTransportDump | DirectTransportDump | PipeTransportDump | PlainTransportDump | WebRtcTransportDump) => BaseTransportDump | DirectTransportDump | PipeTransportDump | PlainTransportDump | WebRtcTransportDump | null): BaseTransportDump | DirectTransportDump | PipeTransportDump | PlainTransportDump | WebRtcTransportDump | null;
export declare function unionListToTransportDumpData(type: TransportDumpData, accessor: (index: number, obj: BaseTransportDump | DirectTransportDump | PipeTransportDump | PlainTransportDump | WebRtcTransportDump) => BaseTransportDump | DirectTransportDump | PipeTransportDump | PlainTransportDump | WebRtcTransportDump | null, index: number): BaseTransportDump | DirectTransportDump | PipeTransportDump | PlainTransportDump | WebRtcTransportDump | null;
//# sourceMappingURL=transport-dump-data.d.ts.map