import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { SctpStreamParameters } from './SctpParameters';
export declare type DataProducerOptions = {
    /**
     * DataProducer id (just for Router.pipeToRouter() method).
     */
    id?: string;
    /**
     * SCTP parameters defining how the endpoint is sending the data.
     */
    sctpStreamParameters: SctpStreamParameters;
    /**
     * A label which can be used to distinguish this DataChannel from others.
     */
    label?: string;
    /**
     * Name of the sub-protocol used by this DataChannel.
     */
    protocol?: string;
    /**
     * Custom application data.
     */
    appData?: any;
};
export declare type DataProducerStat = {
    type: string;
    timestamp: number;
    label: string;
    protocol: string;
    messagesReceived: number;
    bytesReceived: number;
};
export declare class DataProducer extends EnhancedEventEmitter {
    private readonly _internal;
    private readonly _data;
    private readonly _channel;
    private _closed;
    private readonly _appData?;
    private readonly _observer;
    /**
     * @private
     * @emits transportclose
     * @emits @close
     */
    constructor({ internal, data, channel, appData }: {
        internal: any;
        data: any;
        channel: Channel;
        appData: any;
    });
    /**
     * DataProducer id.
     */
    get id(): string;
    /**
     * Whether the DataProducer is closed.
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
     * Close the DataProducer.
     */
    close(): void;
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed(): void;
    /**
     * Dump DataProducer.
     */
    dump(): Promise<any>;
    /**
     * Get DataProducer stats.
     */
    getStats(): Promise<DataProducerStat[]>;
    private _handleWorkerNotifications;
}
//# sourceMappingURL=DataProducer.d.ts.map