import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Transport, TransportListenIp, TransportTuple, SctpState } from './Transport';
import { Consumer, ConsumerOptions } from './Consumer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import { SrtpParameters, SrtpCryptoSuite } from './SrtpParameters';
export declare type PlainTransportOptions = {
    /**
     * Listening IP address.
     */
    listenIp: TransportListenIp | string;
    /**
     * Use RTCP-mux (RTP and RTCP in the same port). Default true.
     */
    rtcpMux?: boolean;
    /**
     * Whether remote IP:port should be auto-detected based on first RTP/RTCP
     * packet received. If enabled, connect() method must not be called. This
     * option is ignored if multiSource is set. Default false.
     */
    comedia?: boolean;
    /**
     * Whether RTP/RTCP from different remote IPs:ports is allowed. If set, the
     * transport will just be valid for receiving media (consume() cannot be
     * called on it) and connect() must not be called. Default false.
     */
    multiSource?: boolean;
    /**
     * Create a SCTP association. Default false.
     */
    enableSctp?: boolean;
    /**
     * SCTP streams number.
     */
    numSctpStreams?: NumSctpStreams;
    /**
     * Maximum size of data that can be passed to DataProducer's send() method.
     * Default 262144.
     */
    maxSctpMessageSize?: number;
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
    probationBytesReceived: number;
    probationRecvBitrate: number;
    probationBytesSent: number;
    probationSendBitrate: number;
    availableOutgoingBitrate?: number;
    availableIncomingBitrate?: number;
    maxIncomingBitrate?: number;
    rtcpMux: boolean;
    comedia: boolean;
    multiSource: boolean;
    tuple: TransportTuple;
    rtcpTuple?: TransportTuple;
};
/**
 * DEPRECATED: Use PlainTransportOptions.
 */
export declare type PlainRtpTransportStat = PlainTransportStat;
export declare class PlainTransport extends Transport {
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
    get sctpState(): SctpState;
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
     * @emits newconsumer - (producer: Producer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     * @emits tuple - (tuple: TransportTuple)
     * @emits rtcptuple - (rtcpTuple: TransportTuple)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    get observer(): EnhancedEventEmitter;
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
    /**
     * Override Transport.consume() method to reject it if multiSource is set.
     *
     * @override
     */
    consume(params: ConsumerOptions): Promise<Consumer>;
    private _handleWorkerNotifications;
}
/**
 * DEPRECATED: Use PlainTransport.
 */
export declare class PlainRtpTransport extends PlainTransport {
    constructor(params: any);
}
//# sourceMappingURL=PlainTransport.d.ts.map