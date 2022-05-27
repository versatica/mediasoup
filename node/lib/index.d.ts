import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Worker, WorkerSettings } from './Worker';
import { RtpCapabilities } from './RtpParameters';
import * as types from './types';
/**
 * Expose all types.
 */
export { types };
/**
 * Expose mediasoup version.
 */
export declare const version = "3.9.13";
/**
 * Expose parseScalabilityMode() function.
 */
export { parse as parseScalabilityMode } from './scalabilityModes';
declare const observer: EnhancedEventEmitter<{
    [x: string]: any[];
}, {
    [x: string]: any[];
} & {
    [x: `@${string}`]: any[];
}>;
/**
 * Observer.
 *
 * @emits newworker - (worker: Worker)
 */
export { observer };
/**
 * Create a Worker.
 */
export declare function createWorker({ logLevel, logTags, rtcMinPort, rtcMaxPort, dtlsCertificateFile, dtlsPrivateKeyFile, appData }?: WorkerSettings): Promise<Worker>;
/**
 * Get a cloned copy of the mediasoup supported RTP capabilities.
 */
export declare function getSupportedRtpCapabilities(): RtpCapabilities;
//# sourceMappingURL=index.d.ts.map