/// <reference types="node" />
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { SctpStreamParameters } from './SctpParameters';
export declare type DataConsumerOptions = {
    /**
     * The id of the DataProducer to consume.
     */
    dataProducerId: string;
    /**
     * Just if consuming over SCTP.
     * Whether data messages must be received in order. If true the messages will
     * be sent reliably. Defaults to the value in the DataProducer if it has type
     * 'sctp' or to true if it has type 'direct'.
     */
    ordered?: boolean;
    /**
     * Just if consuming over SCTP.
     * When ordered is false indicates the time (in milliseconds) after which a
     * SCTP packet will stop being retransmitted. Defaults to the value in the
     * DataProducer if it has type 'sctp' or unset if it has type 'direct'.
     */
    maxPacketLifeTime?: number;
    /**
     * Just if consuming over SCTP.
     * When ordered is false indicates the maximum number of times a packet will
     * be retransmitted. Defaults to the value in the DataProducer if it has type
     * 'sctp' or unset if it has type 'direct'.
     */
    maxRetransmits?: number;
    /**
     * Custom application data.
     */
    appData?: any;
};
export declare type DataConsumerStat = {
    type: string;
    timestamp: number;
    label: string;
    protocol: string;
    messagesSent: number;
    bytesSent: number;
    bufferedAmount: number;
};
/**
 * DataConsumer type.
 */
export declare type DataConsumerType = 'sctp' | 'direct';
export declare type DataConsumerEvents = {
    transportclose: [];
    dataproducerclose: [];
    message: [Buffer, number];
    sctpsendbufferfull: [];
    bufferedamountlow: [number];
};
export declare type DataConsumerObserverEvents = {
    close: [];
};
export declare class DataConsumer extends EnhancedEventEmitter<DataConsumerEvents> {
    #private;
    /**
     * @private
     * @emits transportclose
     * @emits dataproducerclose
     * @emits message - (message: Buffer, ppid: number)
     * @emits sctpsendbufferfull
     * @emits bufferedamountlow - (bufferedAmount: number)
     * @emits @close
     * @emits @dataproducerclose
     */
    constructor({ internal, data, channel, payloadChannel, appData }: {
        internal: any;
        data: any;
        channel: Channel;
        payloadChannel: PayloadChannel;
        appData: any;
    });
    /**
     * DataConsumer id.
     */
    get id(): string;
    /**
     * Associated DataProducer id.
     */
    get dataProducerId(): string;
    /**
     * Whether the DataConsumer is closed.
     */
    get closed(): boolean;
    /**
     * DataConsumer type.
     */
    get type(): DataConsumerType;
    /**
     * SCTP stream parameters.
     */
    get sctpStreamParameters(): SctpStreamParameters | undefined;
    /**
     * DataChannel label.
     */
    get label(): string;
    /**
     * DataChannel protocol.
     */
    get protocol(): string;
    /**
     * App custom data.
     */
    get appData(): any;
    /**
     * Invalid setter.
     */
    set appData(appData: any);
    /**
     * Observer.
     *
     * @emits close
     */
    get observer(): EnhancedEventEmitter<DataConsumerObserverEvents>;
    /**
     * Close the DataConsumer.
     */
    close(): void;
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed(): void;
    /**
     * Dump DataConsumer.
     */
    dump(): Promise<any>;
    /**
     * Get DataConsumer stats.
     */
    getStats(): Promise<DataConsumerStat[]>;
    /**
     * Set buffered amount low threshold.
     */
    setBufferedAmountLowThreshold(threshold: number): Promise<void>;
    /**
     * Send data.
     */
    send(message: string | Buffer, ppid?: number): Promise<void>;
    /**
     * Get buffered amount size.
     */
    getBufferedAmount(): Promise<number>;
    private handleWorkerNotifications;
}
//# sourceMappingURL=DataConsumer.d.ts.map