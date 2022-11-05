import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { TransportProtocol } from './Transport';
import { WebRtcTransport } from './WebRtcTransport';
export declare type WebRtcServerListenInfo = {
    /**
     * Network protocol.
     */
    protocol: TransportProtocol;
    /**
     * Listening IPv4 or IPv6.
     */
    ip: string;
    /**
     * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
     * private IP).
     */
    announcedIp?: string;
    /**
     * Listening port.
     */
    port?: number;
};
export declare type WebRtcServerOptions = {
    /**
     * Listen infos.
     */
    listenInfos: WebRtcServerListenInfo[];
    /**
     * Custom application data.
     */
    appData?: Record<string, unknown>;
};
export declare type WebRtcServerEvents = {
    workerclose: [];
    '@close': [];
};
export declare type WebRtcServerObserverEvents = {
    close: [];
    webrtctransporthandled: [WebRtcTransport];
    webrtctransportunhandled: [WebRtcTransport];
};
declare type WebRtcServerInternal = {
    webRtcServerId: string;
};
export declare class WebRtcServer extends EnhancedEventEmitter<WebRtcServerEvents> {
    #private;
    /**
     * @private
     */
    constructor({ internal, channel, appData }: {
        internal: WebRtcServerInternal;
        channel: Channel;
        appData?: Record<string, unknown>;
    });
    /**
     * WebRtcServer id.
     */
    get id(): string;
    /**
     * Whether the WebRtcServer is closed.
     */
    get closed(): boolean;
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
    get observer(): EnhancedEventEmitter<WebRtcServerObserverEvents>;
    /**
     * @private
     * Just for testing purposes.
     */
    get webRtcTransportsForTesting(): Map<string, WebRtcTransport>;
    /**
     * Close the WebRtcServer.
     */
    close(): void;
    /**
     * Worker was closed.
     *
     * @private
     */
    workerClosed(): void;
    /**
     * Dump WebRtcServer.
     */
    dump(): Promise<any>;
    /**
     * @private
     */
    handleWebRtcTransport(webRtcTransport: WebRtcTransport): void;
}
export {};
//# sourceMappingURL=WebRtcServer.d.ts.map