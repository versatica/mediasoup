/// <reference types="node" />
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { SctpStreamParameters } from './SctpParameters';
export declare type DataProducerOptions = {
    /**
     * DataProducer id (just for Router.pipeToRouter() method).
     */
    id?: string;
    /**
     * SCTP parameters defining how the endpoint is sending the data.
     * Just if messages are sent over SCTP.
     */
    sctpStreamParameters?: SctpStreamParameters;
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
/**
 * DataProducer type.
 */
export declare type DataProducerType = 'sctp' | 'direct';
export declare type DataProducerEvents = {
    transportclose: [];
};
export declare type DataProducerObserverEvents = {
    close: [];
};
export declare class DataProducer extends EnhancedEventEmitter<DataProducerEvents> {
    #private;
    /**
     * @private
     * @emits transportclose
     * @emits @close
     */
    constructor({ internal, data, channel, payloadChannel, appData }: {
        internal: any;
        data: any;
        channel: Channel;
        payloadChannel: PayloadChannel;
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
     * DataProducer type.
     */
    get type(): DataProducerType;
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
    get observer(): EnhancedEventEmitter<DataProducerObserverEvents>;
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
    /**
     * Send data (just valid for DataProducers created on a DirectTransport).
     */
    send(message: string | Buffer, ppid?: number): void;
    private handleWorkerNotifications;
}
//# sourceMappingURL=DataProducer.d.ts.map