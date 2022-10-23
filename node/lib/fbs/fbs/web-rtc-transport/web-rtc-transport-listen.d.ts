import { WebRtcTransportListenIndividual } from '../../fbs/web-rtc-transport/web-rtc-transport-listen-individual';
import { WebRtcTransportListenServer } from '../../fbs/web-rtc-transport/web-rtc-transport-listen-server';
export declare enum WebRtcTransportListen {
    NONE = 0,
    WebRtcTransportListenIndividual = 1,
    WebRtcTransportListenServer = 2
}
export declare function unionToWebRtcTransportListen(type: WebRtcTransportListen, accessor: (obj: WebRtcTransportListenIndividual | WebRtcTransportListenServer) => WebRtcTransportListenIndividual | WebRtcTransportListenServer | null): WebRtcTransportListenIndividual | WebRtcTransportListenServer | null;
export declare function unionListToWebRtcTransportListen(type: WebRtcTransportListen, accessor: (index: number, obj: WebRtcTransportListenIndividual | WebRtcTransportListenServer) => WebRtcTransportListenIndividual | WebRtcTransportListenServer | null, index: number): WebRtcTransportListenIndividual | WebRtcTransportListenServer | null;
//# sourceMappingURL=web-rtc-transport-listen.d.ts.map