import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { RtpObserver, RtpObserverEvents, RtpObserverObserverEvents, RtpObserverObserverInternal } from './RtpObserver';
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
        producer?: Producer;
    }];
};
export declare type ActiveSpeakerObserverObserverEvents = RtpObserverObserverEvents & {
    dominantspeaker: [{
        producer?: Producer;
    }];
};
export declare class ActiveSpeakerObserver extends RtpObserver<ActiveSpeakerObserverEvents> {
    /**
     * @private
     */
    constructor({ internal, channel, payloadChannel, appData, getProducerById }: {
        internal: RtpObserverObserverInternal;
        channel: Channel;
        payloadChannel: PayloadChannel;
        appData?: Record<string, unknown>;
        getProducerById: (producerId: string) => Producer | undefined;
    });
    /**
     * Observer.
     */
    get observer(): EnhancedEventEmitter<ActiveSpeakerObserverObserverEvents>;
    private handleWorkerNotifications;
}
//# sourceMappingURL=ActiveSpeakerObserver.d.ts.map