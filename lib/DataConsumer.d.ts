import EnhancedEventEmitter from './EnhancedEventEmitter';
import Channel from './Channel';
import { SctpStreamParameters } from './SctpParameters';
export interface DataConsumerOptions {
    /**
     * The id of the DataProducer to consume.
     */
    dataProducerId: string;
    /**
     * Custom application data.
     */
    appData?: any;
}
export interface DataConsumerStat {
    type: string;
    timestamp: number;
    label: string;
    protocol: string;
    messagesSent: number;
    bytesSent: number;
}
export default class DataConsumer extends EnhancedEventEmitter {
    private readonly _internal;
    private readonly _data;
    private readonly _channel;
    private _closed;
    private readonly _appData?;
    private readonly _observer;
    /**
     * @private
     * @emits transportclose
     * @emits dataproducerclose
     * @emits @close
     * @emits @dataproducerclose
     */
    constructor({ internal, data, channel, appData }: {
        internal: any;
        data: any;
        channel: Channel;
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
     * SCTP stream parameters.
     */
    get sctpStreamParameters(): SctpStreamParameters;
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
    get observer(): EnhancedEventEmitter;
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
    private _handleWorkerNotifications;
}
//# sourceMappingURL=DataConsumer.d.ts.map