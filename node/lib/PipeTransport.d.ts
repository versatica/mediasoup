import { Transport, TransportListenIp, TransportTuple, TransportEvents, TransportObserverEvents, SctpState } from './Transport';
import { Consumer } from './Consumer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import { SrtpParameters } from './SrtpParameters';
export declare type PipeTransportOptions = {
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
     * Create a SCTP association. Default false.
     */
    enableSctp?: boolean;
    /**
     * SCTP streams number.
     */
    numSctpStreams?: NumSctpStreams;
    /**
     * Maximum allowed size for SCTP messages sent by DataProducers.
     * Default 268435456.
     */
    maxSctpMessageSize?: number;
    /**
     * Maximum SCTP send buffer used by DataConsumers.
     * Default 268435456.
     */
    sctpSendBufferSize?: number;
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
    probationBytesSent: number;
    probationSendBitrate: number;
    availableOutgoingBitrate?: number;
    availableIncomingBitrate?: number;
    maxIncomingBitrate?: number;
    tuple: TransportTuple;
};
export declare type PipeConsumerOptions = {
    /**
     * The id of the Producer to consume.
     */
    producerId: string;
    /**
     * Custom application data.
     */
    appData?: any;
};
export declare type PipeTransportEvents = TransportEvents & {
    sctpstatechange: [SctpState];
};
export declare type PipeTransportObserverEvents = TransportObserverEvents & {
    sctpstatechange: [SctpState];
};
export declare class PipeTransport extends Transport<PipeTransportEvents, PipeTransportObserverEvents> {
    #private;
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
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    /**
     * Close the PipeTransport.
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
    consume({ producerId, appData }: PipeConsumerOptions): Promise<Consumer>;
    private handleWorkerNotifications;
}
//# sourceMappingURL=PipeTransport.d.ts.map