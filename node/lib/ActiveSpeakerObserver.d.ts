import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { RtpObserver, RtpObserverEvents, RtpObserverObserverEvents, RtpObserverConstructorOptions } from './RtpObserver';
import { Producer } from './Producer';
export interface ActiveSpeakerObserverOptions {
    interval?: number;
    /**
     * Custom application data.
     */
    appData?: Record<string, unknown>;
}
export interface ActiveSpeakerObserverActivity {
    /**
     * The producer instance.
     */
    producer: Producer;
}
export declare type ActiveSpeakerObserverEvents = RtpObserverEvents & {
    dominantspeaker: [{
        producer: Producer;
    }];
};
export declare type ActiveSpeakerObserverObserverEvents = RtpObserverObserverEvents & {
    dominantspeaker: [{
        producer: Producer;
    }];
};
declare type RtpObserverObserverConstructorOptions = RtpObserverConstructorOptions;
export declare class ActiveSpeakerObserver extends RtpObserver<ActiveSpeakerObserverEvents> {
    /**
     * @private
     */
    constructor(options: RtpObserverObserverConstructorOptions);
    /**
     * Observer.
     */
    get observer(): EnhancedEventEmitter<ActiveSpeakerObserverObserverEvents>;
    private handleWorkerNotifications;
}
export {};
//# sourceMappingURL=ActiveSpeakerObserver.d.ts.map