/// <reference types="node" />
import { Transport, TransportEvents, TransportObserverEvents, TransportConstructorOptions } from './Transport';
import { SctpParameters } from './SctpParameters';
export declare type DirectTransportOptions = {
    /**
     * Maximum allowed size for direct messages sent from DataProducers.
     * Default 262144.
     */
    maxMessageSize: number;
    /**
     * Custom application data.
     */
    appData?: Record<string, unknown>;
};
export declare type DirectTransportStat = {
    type: string;
    transportId: string;
    timestamp: number;
    bytesReceived: number;
    recvBitrate: number;
    bytesSent: number;
    sendBitrate: number;
    rtpBytesReceived: number;
    rtpRecvBitrate: number;
    rtpBytesSent: number;
    rtpSendBitrate: number;
    rtxBytesReceived: number;
    rtxRecvBitrate: number;
    rtxBytesSent: number;
    rtxSendBitrate: number;
    probationBytesSent: number;
    probationSendBitrate: number;
    availableOutgoingBitrate?: number;
    availableIncomingBitrate?: number;
    maxIncomingBitrate?: number;
};
export declare type DirectTransportEvents = TransportEvents & {
    rtcp: [Buffer];
};
export declare type DirectTransportObserverEvents = TransportObserverEvents & {
    rtcp: [Buffer];
};
declare type DirectTransportConstructorOptions = TransportConstructorOptions & {
    data: DirectTransportData;
};
export declare type DirectTransportData = {
    sctpParameters?: SctpParameters;
};
export declare class DirectTransport extends Transport<DirectTransportEvents, DirectTransportObserverEvents> {
    #private;
    /**
     * @private
     */
    constructor(options: DirectTransportConstructorOptions);
    /**
     * Close the DirectTransport.
     *
     * @override
     */
    close(): void;
    /**
     * Router was closed.
     *
     * @private
     * @override
     */
    routerClosed(): void;
    /**
     * Get DirectTransport stats.
     *
     * @override
     */
    getStats(): Promise<DirectTransportStat[]>;
    /**
     * NO-OP method in DirectTransport.
     *
     * @override
     */
    connect(): Promise<void>;
    /**
     * @override
     */
    setMaxIncomingBitrate(bitrate: number): Promise<void>;
    /**
     * @override
     */
    setMaxOutgoingBitrate(bitrate: number): Promise<void>;
    /**
     * Send RTCP packet.
     */
    sendRtcp(rtcpPacket: Buffer): void;
    private handleWorkerNotifications;
}
export {};
//# sourceMappingURL=DirectTransport.d.ts.map