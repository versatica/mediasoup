import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { RtpObserver, RtpObserverEvents, RtpObserverObserverEvents, RtpObserverConstructorOptions } from './RtpObserver';
import { Producer } from './Producer';
export declare type ActiveSpeakerObserverOptions = {
    interval?: number;
    /**
     * Custom application data.
     */
    appData?: Record<string, unknown>;
};
export declare type ActiveSpeakerObserverDominantSpeaker = {
    /**
     * The audio Producer instance.
     */
    producer: Producer;
};
export declare type ActiveSpeakerObserverEvents = RtpObserverEvents & {
    dominantspeaker: [ActiveSpeakerObserverDominantSpeaker];
};
export declare type ActiveSpeakerObserverObserverEvents = RtpObserverObserverEvents & {
    dominantspeaker: [ActiveSpeakerObserverDominantSpeaker];
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