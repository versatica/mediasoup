import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { RouterInternal } from './Router';
import { Producer } from './Producer';
export declare type RtpObserverEvents = {
    routerclose: [];
    '@close': [];
};
export declare type RtpObserverObserverEvents = {
    close: [];
    pause: [];
    resume: [];
    addproducer: [Producer];
    removeproducer: [Producer];
};
export declare type RtpObserverConstructorOptions = {
    internal: RtpObserverObserverInternal;
    channel: Channel;
    payloadChannel: PayloadChannel;
    appData?: Record<string, unknown>;
    getProducerById: (producerId: string) => Producer | undefined;
};
export declare type RtpObserverObserverInternal = RouterInternal & {
    rtpObserverId: string;
};
export declare type RtpObserverAddRemoveProducerOptions = {
    /**
     * The id of the Producer to be added or removed.
     */
    producerId: string;
};
export declare class RtpObserver<E extends RtpObserverEvents = RtpObserverEvents> extends EnhancedEventEmitter<E> {
    #private;
    protected readonly internal: RtpObserverObserverInternal;
    protected readonly channel: Channel;
    protected readonly payloadChannel: PayloadChannel;
    protected readonly getProducerById: (producerId: string) => Producer | undefined;
    /**
     * @private
     * @interface
     */
    constructor({ internal, channel, payloadChannel, appData, getProducerById }: RtpObserverConstructorOptions);
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
    get appData(): Record<string, unknown>;
    /**
     * Invalid setter.
     */
    set appData(appData: Record<string, unknown>);
    /**
     * Observer.
     */
    get observer(): EnhancedEventEmitter<RtpObserverObserverEvents>;
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
//# sourceMappingURL=RtpObserver.d.ts.map