/// <reference types="node" />
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Transport } from './Transport';
export declare type DirectTransportOptions = {
    /**
     * Maximum allowed size for direct messages sent from DataProducers.
     * Default 262144.
     */
    maxMessageSize: number;
    /**
     * Custom application data.
     */
    appData?: any;
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
export declare class DirectTransport extends Transport {
    protected readonly _data: {};
    /**
     * @private
     * @emits rtcp - (packet: Buffer)
     * @emits trace - (trace: TransportTraceEventData)
     */
    constructor(params: any);
    /**
     * Observer.
     *
     * @override
     * @emits close
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     * @emits trace - (trace: TransportTraceEventData)
     */
    get observer(): EnhancedEventEmitter;
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
     * Send RTCP packet.
     */
    sendRtcp(rtcpPacket: Buffer): void;
    private _handleWorkerNotifications;
}
//# sourceMappingURL=DirectTransport.d.ts.map