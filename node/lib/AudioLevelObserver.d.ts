import { RtpObserver, RtpObserverEvents, RtpObserverObserverEvents } from './RtpObserver';
import { Producer } from './Producer';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
export interface AudioLevelObserverOptions {
    /**
     * Maximum number of entries in the 'volumes”' event. Default 1.
     */
    maxEntries?: number;
    /**
     * Minimum average volume (in dBvo from -127 to 0) for entries in the
     * 'volumes' event.	Default -80.
     */
    threshold?: number;
    /**
     * Interval in ms for checking audio volumes. Default 1000.
     */
    interval?: number;
    /**
     * Custom application data.
     */
    appData?: any;
}
export interface AudioLevelObserverVolume {
    /**
     * The audio producer instance.
     */
    producer: Producer;
    /**
     * The average volume (in dBvo from -127 to 0) of the audio producer in the
     * last interval.
     */
    volume: number;
}
export declare type AudioLevelObserverEvents = RtpObserverEvents & {
    volumes: [AudioLevelObserverVolume[]];
    silence: [];
};
export declare type AudioLevelObserverObserverEvents = RtpObserverObserverEvents & {
    volumes: [AudioLevelObserverVolume[]];
    silence: [];
};
export declare class AudioLevelObserver extends RtpObserver<AudioLevelObserverEvents> {
    /**
     * @private
     * @emits volumes - (volumes: AudioLevelObserverVolume[])
     * @emits silence
     */
    constructor(params: any);
    /**
     * Observer.
     *
     * @emits close
     * @emits pause
     * @emits resume
     * @emits addproducer - (producer: Producer)
     * @emits removeproducer - (producer: Producer)
     * @emits volumes - (volumes: AudioLevelObserverVolume[])
     * @emits silence
     */
    get observer(): EnhancedEventEmitter<AudioLevelObserverObserverEvents>;
    private handleWorkerNotifications;
}
//# sourceMappingURL=AudioLevelObserver.d.ts.map