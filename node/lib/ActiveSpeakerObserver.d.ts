import { RtpObserver, RtpObserverEvents } from './RtpObserver';
import { Producer } from './Producer';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
export interface ActiveSpeakerObserverOptions {
    interval?: number;
    /**
     * Custom application data.
     */
    appData?: any;
}
export interface ActiveSpeakerObserverActivity {
    /**
     * The producer instance.
     */
    producer: Producer;
}
declare type ObserverEvents = RtpObserverEvents & {
    dominantspeaker: [{
        producer: Producer;
    }];
};
declare type Events = {
    routerclose: [];
    dominantspeaker: [{
        producer: Producer;
    }];
};
export declare class ActiveSpeakerObserver extends RtpObserver<Events> {
    /**
     * @private
     */
    constructor(params: any);
    /**
     * Observer.
     */
    get observer(): EnhancedEventEmitter<ObserverEvents>;
    private handleWorkerNotifications;
}
export {};
//# sourceMappingURL=ActiveSpeakerObserver.d.ts.map