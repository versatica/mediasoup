import { Transport, TransportListenIp, TransportProtocol, TransportTuple, TransportEvents, TransportObserverEvents, SctpState } from './Transport';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
export declare type WebRtcTransportOptions = {
    /**
     * Listening IP address or addresses in order of preference (first one is the
     * preferred one).
     */
    listenIps: (TransportListenIp | string)[];
    /**
     * Fixed port to listen on instead of selecting automatically from Worker's port
     * range.
     */
    port?: number;
    /**
     * Listen in UDP. Default true.
     */
    enableUdp?: boolean;
    /**
     * Listen in TCP. Default false.
     */
    enableTcp?: boolean;
    /**
     * Prefer UDP. Default false.
     */
    preferUdp?: boolean;
    /**
     * Prefer TCP. Default false.
     */
    preferTcp?: boolean;
    /**
     * Initial available outgoing bitrate (in bps). Default 600000.
     */
    initialAvailableOutgoingBitrate?: number;
    /**
     * Create a SCTP association. Default false.
     */
    enableSctp?: boolean;
    /**
     * SCTP streams number.
     */
    numSctpStreams?: NumSctpStreams;
    /**
     * Maximum allowed size for SCTP messages sent by DataProducers.
     * Default 262144.
     */
    maxSctpMessageSize?: number;
    /**
     * Maximum SCTP send buffer used by DataConsumers.
     * Default 262144.
     */
    sctpSendBufferSize?: number;
    /**
     * Custom application data.
     */
    appData?: any;
};
export declare type IceParameters = {
    usernameFragment: string;
    password: string;
    iceLite?: boolean;
};
export declare type IceCandidate = {
    foundation: string;
    priority: number;
    ip: string;
    protocol: TransportProtocol;
    port: number;
    type: 'host';
    tcpType: 'passive' | undefined;
};
export declare type DtlsParameters = {
    role?: DtlsRole;
    fingerprints: DtlsFingerprint[];
};
/**
 * The hash function algorithm (as defined in the "Hash function Textual Names"
 * registry initially specified in RFC 4572 Section 8) and its corresponding
 * certificate fingerprint value (in lowercase hex string as expressed utilizing
 * the syntax of "fingerprint" in RFC 4572 Section 5).
 */
export declare type DtlsFingerprint = {
    algorithm: string;
    value: string;
};
export declare type IceState = 'new' | 'connected' | 'completed' | 'disconnected' | 'closed';
export declare type DtlsRole = 'auto' | 'client' | 'server';
export declare type DtlsState = 'new' | 'connecting' | 'connected' | 'failed' | 'closed';
export declare type WebRtcTransportStat = {
    type: string;
    transportId: string;
    timestamp: number;
    sctpState?: SctpState;
    bytesReceived: number;
    recvBitrate: number;
    bytesSent: number;
    sendBitrate: number;
    rtpBytesReceived: number;
    rtpRecvBitrate: number;
    rtpBytesSent: number;
    rtpSendBitrate: number;
    rtxBytesReceived: number;
    rtxRecvBitrate: number;
    rtxBytesSent: number;
    rtxSendBitrate: number;
    probationBytesSent: number;
    probationSendBitrate: number;
    availableOutgoingBitrate?: number;
    availableIncomingBitrate?: number;
    maxIncomingBitrate?: number;
    iceRole: string;
    iceState: IceState;
    iceSelectedTuple?: TransportTuple;
    dtlsState: DtlsState;
};
export declare type WebRtcTransportEvents = TransportEvents & {
    icestatechange: [IceState];
    iceselectedtuplechange: [TransportTuple];
    dtlsstatechange: [DtlsState];
    sctpstatechange: [SctpState];
};
export declare type WebRtcTransportObserverEvents = TransportObserverEvents & {
    icestatechange: [IceState];
    iceselectedtuplechange: [TransportTuple];
    dtlsstatechange: [DtlsState];
    sctpstatechange: [SctpState];
};
export declare class WebRtcTransport extends Transport<WebRtcTransportEvents, WebRtcTransportObserverEvents> {
    #private;
    /**
     * @private
     * @emits icestatechange - (iceState: IceState)
     * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
     * @emits dtlsstatechange - (dtlsState: DtlsState)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params: any);
    /**
     * ICE role.
     */
    get iceRole(): 'controlled';
    /**
     * ICE parameters.
     */
    get iceParameters(): IceParameters;
    /**
     * ICE candidates.
     */
    get iceCandidates(): IceCandidate[];
    /**
     * ICE state.
     */
    get iceState(): IceState;
    /**
     * ICE selected tuple.
     */
    get iceSelectedTuple(): TransportTuple | undefined;
    /**
     * DTLS parameters.
     */
    get dtlsParameters(): DtlsParameters;
    /**
     * DTLS state.
     */
    get dtlsState(): DtlsState;
    /**
     * Remote certificate in PEM format.
     */
    get dtlsRemoteCert(): string | undefined;
    /**
     * SCTP parameters.
     */
    get sctpParameters(): SctpParameters | undefined;
    /**
     * SCTP state.
     */
    get sctpState(): SctpState | undefined;
    /**
     * Observer.
     *
     * @override
     * @emits close
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (consumer: Consumer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataConsumer: DataConsumer)
     * @emits icestatechange - (iceState: IceState)
     * @emits iceselectedtuplechange - (iceSelectedTuple: TransportTuple)
     * @emits dtlsstatechange - (dtlsState: DtlsState)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    /**
     * Close the WebRtcTransport.
     *
     * @override
     */
    close(): void;
    /**
     * Router was closed.
     *
     * @private
     * @override
     */
    routerClosed(): void;
    /**
     * Get WebRtcTransport stats.
     *
     * @override
     */
    getStats(): Promise<WebRtcTransportStat[]>;
    /**
     * Provide the WebRtcTransport remote parameters.
     *
     * @override
     */
    connect({ dtlsParameters }: {
        dtlsParameters: DtlsParameters;
    }): Promise<void>;
    /**
     * Restart ICE.
     */
    restartIce(): Promise<IceParameters>;
    private handleWorkerNotifications;
}
//# sourceMappingURL=WebRtcTransport.d.ts.map