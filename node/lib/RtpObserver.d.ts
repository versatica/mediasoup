import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { Producer } from './Producer';
export declare type RtpObserverEvents = {
    close: [];
    pause: [];
    resume: [];
    addproducer: [Producer];
    removeproducer: [Producer];
};
declare type Events = {
    routerclose: [];
};
export declare type RtpObserverAddRemoveProducerOptions = {
    /**
     * The id of the Producer to be added or removed.
     */
    producerId: string;
};
export declare class RtpObserver<E extends Events = Events> extends EnhancedEventEmitter<E> {
    #private;
    protected readonly internal: {
        routerId: string;
        rtpObserverId: string;
    };
    protected readonly channel: Channel;
    protected readonly payloadChannel: PayloadChannel;
    protected readonly getProducerById: (producerId: string) => Producer;
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     */
    constructor({ internal, channel, payloadChannel, appData, getProducerById }: {
        internal: any;
        channel: Channel;
        payloadChannel: PayloadChannel;
        appData: any;
        getProducerById: (producerId: string) => Producer;
    });
    /**
     * RtpObserver id.
     */
    get id(): string;
    /**
     * Whether the RtpObserver is closed.
     */
    get closed(): boolean;
    /**
     * Whether the RtpObserver is paused.
     */
    get paused(): boolean;
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
     * @emits pause
     * @emits resume
     * @emits addproducer - (producer: Producer)
     * @emits removeproducer - (producer: Producer)
     */
    get observer(): EnhancedEventEmitter<RtpObserverEvents>;
    /**
     * Close the RtpObserver.
     */
    close(): void;
    /**
     * Router was closed.
     *
     * @private
     */
    routerClosed(): void;
    /**
     * Pause the RtpObserver.
     */
    pause(): Promise<void>;
    /**
     * Resume the RtpObserver.
     */
    resume(): Promise<void>;
    /**
     * Add a Producer to the RtpObserver.
     */
    addProducer({ producerId }: RtpObserverAddRemoveProducerOptions): Promise<void>;
    /**
     * Remove a Producer from the RtpObserver.
     */
    removeProducer({ producerId }: RtpObserverAddRemoveProducerOptions): Promise<void>;
}
export {};
//# sourceMappingURL=RtpObserver.d.ts.map