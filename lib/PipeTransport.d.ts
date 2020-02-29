import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Transport, TransportListenIp, TransportTuple, SctpState } from './Transport';
import { Consumer, ConsumerOptions } from './Consumer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import { SrtpParameters } from './SrtpParameters';
export declare type PipeTransportOptions = {
    /**
     * Listening IP address.
     */
    listenIp: TransportListenIp | string;
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
     * Enable RTX and NACK for RTP retransmission. Useful if both Routers are
     * located in different hosts and there is packet lost in the link. For this
     * to work, both PipeTransports must enable this setting. Default false.
     */
    enableRtx?: boolean;
    /**
     * Enable SRTP. Useful to protect the RTP and RTCP traffic if both Routers
     * are located in different hosts. For this to work, connect() must be called
     * with remote SRTP parameters. Default false.
     */
    enableSrtp?: boolean;
    /**
     * Custom application data.
     */
    appData?: any;
};
export declare type PipeTransportStat = {
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
    tuple: TransportTuple;
};
export declare class PipeTransport extends Transport {
    /**
     * @private
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params: any);
    /**
     * Transport tuple.
     */
    get tuple(): TransportTuple;
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
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    get observer(): EnhancedEventEmitter;
    /**
     * Close the PlainRtpTransport.
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
     * Get PipeTransport stats.
     *
     * @override
     */
    getStats(): Promise<PipeTransportStat[]>;
    /**
     * Provide the PipeTransport remote parameters.
     *
     * @override
     */
    connect({ ip, port, srtpParameters }: {
        ip: string;
        port: number;
        srtpParameters?: SrtpParameters;
    }): Promise<void>;
    /**
     * Create a pipe Consumer.
     *
     * @override
     */
    consume({ producerId, appData }: ConsumerOptions): Promise<Consumer>;
    private _handleWorkerNotifications;
}
//# sourceMappingURL=PipeTransport.d.ts.map