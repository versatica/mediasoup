import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Transport, SctpState } from './Transport';
import { Producer, ProducerOptions } from './Producer';
import { Consumer, ConsumerOptions } from './Consumer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
export declare type DataTransportOptions = {
    /**
     * Create a SCTP association. Default true.
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
     * Custom application data.
     */
    appData?: any;
};
export declare type DataTransportStat = {
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
};
export declare class DataTransport extends Transport {
    protected readonly _data: {
        sctpParameters?: SctpParameters;
        sctpState?: SctpState;
    };
    /**
     * @private
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params: any);
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
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     * @emits sctpstatechange - (sctpState: SctpState)
     * @emits trace - (trace: TransportTraceEventData)
     */
    get observer(): EnhancedEventEmitter;
    /**
     * Close the DataTransport.
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
     * Get DataTransport stats.
     *
     * @override
     */
    getStats(): Promise<DataTransportStat[]>;
    /**
     * NO-OP method in DataTransport.
     *
     * @override
     */
    connect(): Promise<void>;
    /**
     * @override
     */
    setMaxIncomingBitrate(bitrate: number): Promise<void>;
    /**
     * @override
     */
    produce(options: ProducerOptions): Promise<Producer>;
    /**
     * @override
     */
    consumer(options: ConsumerOptions): Promise<Consumer>;
    private _handleWorkerNotifications;
}
//# sourceMappingURL=DataTransport.d.ts.map