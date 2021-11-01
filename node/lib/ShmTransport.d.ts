import { Consumer, ConsumerOptions } from './Consumer';
import { Transport, TransportListenIp } from './Transport';
export interface ShmTransportOptions {
    /**
     * Listening IP address.
     */
    listenIp: TransportListenIp | string;
    shm: any;
    log: any;
    /**
     * Custom application data.
     */
    appData?: any;
}
export interface ShmTransportStat {
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
    probationBytesReceived: number;
    probationRecvBitrate: number;
    probationBytesSent: number;
    probationSendBitrate: number;
    availableOutgoingBitrate?: number;
    availableIncomingBitrate?: number;
    maxIncomingBitrate?: number;
    shm: string;
    writer: number;
}
export declare class ShmTransport extends Transport {
    private _shm?;
    private _log?;
    /**
    * @private
     *
     */
    constructor(params: any);
    /**
     * Observer.
     *
     * @override
     * @type {EventEmitter}
     *
     * @emits close
     * @emits {producer: Producer} newproducer
     * @emits {consumer: Consumer} newconsumer
     * @emits {producer: DataProducer} newdataproducer
     * @emits {consumer: DataConsumer} newdataconsumer
     * @emits {sctpState: String} sctpstatechange
     */
    /**
     * Close the ShmTransport.
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
     * Get ShmTransport stats.
     *
     * @override
     */
    getStats(): Promise<ShmTransportStat[]>;
    /**
     * Provide the ShmTransport remote parameters.
     *
     * @param {String} shm- shm name.
     *
     * @async
     * @override
     */
    connect({ shm }: {
        shm: string;
    }): Promise<void>;
    /**
     * Create a shm Consumer.
     *
     * @virtual
     */
    consume({ producerId, rtpCapabilities, paused, preferredLayers, pipe, appData }: ConsumerOptions): Promise<Consumer>;
    /**
     * Provide the ShmTransport remote parameters.
     *
     * @param {Object} meta - metadata string.
     *
     * @async
     */
    writeStreamMetaData({ meta }: {
        meta: string;
    }): Promise<void>;
    /**
     * Does nothing, should not be called like this
     *
     * @private
     * @override
     */
    _handleWorkerNotifications(): void;
}
//# sourceMappingURL=ShmTransport.d.ts.map