import { Transport, TransportListenIp, TransportTuple, TransportEvents, TransportObserverEvents, SctpState } from './Transport';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import { SrtpParameters, SrtpCryptoSuite } from './SrtpParameters';
export declare type PlainTransportOptions = {
    /**
     * Listening IP address.
     */
    listenIp: TransportListenIp | string;
    /**
     * Fixed port to listen on instead of selecting automatically from Worker's port
     * range.
     */
    port?: number;
    /**
     * Use RTCP-mux (RTP and RTCP in the same port). Default true.
     */
    rtcpMux?: boolean;
    /**
     * Whether remote IP:port should be auto-detected based on first RTP/RTCP
     * packet received. If enabled, connect() method must not be called unless
     * SRTP is enabled. If so, it must be called with just remote SRTP parameters.
     * Default false.
     */
    comedia?: boolean;
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
     * Enable SRTP. For this to work, connect() must be called
     * with remote SRTP parameters. Default false.
     */
    enableSrtp?: boolean;
    /**
     * The SRTP crypto suite to be used if enableSrtp is set. Default
     * 'AES_CM_128_HMAC_SHA1_80'.
     */
    srtpCryptoSuite?: SrtpCryptoSuite;
    /**
     * Custom application data.
     */
    appData?: any;
};
/**
 * DEPRECATED: Use PlainTransportOptions.
 */
export declare type PlainRtpTransportOptions = PlainTransportOptions;
export declare type PlainTransportStat = {
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
    rtcpMux: boolean;
    comedia: boolean;
    tuple: TransportTuple;
    rtcpTuple?: TransportTuple;
};
/**
 * DEPRECATED: Use PlainTransportStat.
 */
export declare type PlainRtpTransportStat = PlainTransportStat;
export declare type PlainTransportEvents = TransportEvents & {
    tuple: [TransportTuple];
    rtcptuple: [TransportTuple];
    sctpstatechange: [SctpState];
};
export declare type PlainTransportObserverEvents = TransportObserverEvents & {
    tuple: [TransportTuple];
    rtcptuple: [TransportTuple];
    sctpstatechange: [SctpState];
};
export declare class PlainTransport extends Transport<PlainTransportEvents, PlainTransportObserverEvents> {
    #private;
    /**
     * @private
     * @emits tuple - (tuple: TransportTuple)
     * @emits rtcptuple - (rtcpTuple: TransportTuple)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params: any);
    /**
     * Transport tuple.
     */
    get tuple(): TransportTuple;
    /**
     * Transport RTCP tuple.
     */
    get rtcpTuple(): TransportTuple | undefined;
    /**
     * SCTP parameters.
     */
    get sctpParameters(): SctpParameters | undefined;
    /**
     * SCTP state.
     */
    get sctpState(): SctpState | undefined;
    /**
     * SRTP parameters.
     */
    get srtpParameters(): SrtpParameters | undefined;
    /**
     * Observer.
     *
     * @override
     * @emits close
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (consumer: Consumer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataConsumer: DataConsumer)
     * @emits tuple - (tuple: TransportTuple)
     * @emits rtcptuple - (rtcpTuple: TransportTuple)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    /**
     * Close the PlainTransport.
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
     * Get PlainTransport stats.
     *
     * @override
     */
    getStats(): Promise<PlainTransportStat[]>;
    /**
     * Provide the PlainTransport remote parameters.
     *
     * @override
     */
    connect({ ip, port, rtcpPort, srtpParameters }: {
        ip?: string;
        port?: number;
        rtcpPort?: number;
        srtpParameters?: SrtpParameters;
    }): Promise<void>;
    private handleWorkerNotifications;
}
/**
 * DEPRECATED: Use PlainTransport.
 */
export declare class PlainRtpTransport extends PlainTransport {
    constructor(params: any);
}
//# sourceMappingURL=PlainTransport.d.ts.map