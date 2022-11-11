import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { RtpObserver, RtpObserverEvents, RtpObserverObserverEvents, RtpObserverConstructorOptions } from './RtpObserver';
import { Producer } from './Producer';
export declare type AudioLevelObserverOptions = {
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
    appData?: Record<string, unknown>;
};
export declare type AudioLevelObserverVolume = {
    /**
     * The audio Producer instance.
     */
    producer: Producer;
    /**
     * The average volume (in dBvo from -127 to 0) of the audio Producer in the
     * last interval.
     */
    volume: number;
};
export declare type AudioLevelObserverEvents = RtpObserverEvents & {
    volumes: [AudioLevelObserverVolume[]];
    silence: [];
};
export declare type AudioLevelObserverObserverEvents = RtpObserverObserverEvents & {
    volumes: [AudioLevelObserverVolume[]];
    silence: [];
};
declare type AudioLevelObserverConstructorOptions = RtpObserverConstructorOptions;
export declare class AudioLevelObserver extends RtpObserver<AudioLevelObserverEvents> {
    /**
     * @private
     */
    constructor(options: AudioLevelObserverConstructorOptions);
    /**
     * Observer.
     */
    get observer(): EnhancedEventEmitter<AudioLevelObserverObserverEvents>;
    private handleWorkerNotifications;
}
export {};
//# sourceMappingURL=AudioLevelObserver.d.ts.map