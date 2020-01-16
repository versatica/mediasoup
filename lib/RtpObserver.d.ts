import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { Producer } from './Producer';
export declare class RtpObserver extends EnhancedEventEmitter {
    protected readonly _internal: any;
    protected readonly _channel: Channel;
    protected _closed: boolean;
    protected _paused: boolean;
    private readonly _appData?;
    protected readonly _getProducerById: (producerId: string) => Producer;
    protected readonly _observer: EnhancedEventEmitter;
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     */
    constructor({ internal, channel, appData, getProducerById }: {
        internal: any;
        channel: Channel;
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
    get observer(): EnhancedEventEmitter;
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
    addProducer({ producerId }: {
        producerId: string;
    }): Promise<void>;
    /**
     * Remove a Producer from the RtpObserver.
     */
    removeProducer({ producerId }: {
        producerId: string;
    }): Promise<void>;
}
//# sourceMappingURL=RtpObserver.d.ts.map