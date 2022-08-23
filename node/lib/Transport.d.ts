import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { RouterInternal } from './Router';
import { WebRtcTransportData } from './WebRtcTransport';
import { PlainTransportData } from './PlainTransport';
import { PipeTransportData } from './PipeTransport';
import { DirectTransportData } from './DirectTransport';
import { Producer, ProducerOptions } from './Producer';
import { Consumer, ConsumerOptions } from './Consumer';
import { DataProducer, DataProducerOptions } from './DataProducer';
import { DataConsumer, DataConsumerOptions } from './DataConsumer';
import { RtpCapabilities } from './RtpParameters';
export interface TransportListenIp {
    /**
     * Listening IPv4 or IPv6.
     */
    ip: string;
    /**
     * Announced IPv4 or IPv6 (useful when running mediasoup behind NAT with
     * private IP).
     */
    announcedIp?: string;
}
/**
 * Transport protocol.
 */
export declare type TransportProtocol = 'udp' | 'tcp';
export interface TransportTuple {
    localIp: string;
    localPort: number;
    remoteIp?: string;
    remotePort?: number;
    protocol: TransportProtocol;
}
/**
 * Valid types for 'trace' event.
 */
export declare type TransportTraceEventType = 'probation' | 'bwe';
/**
 * 'trace' event data.
 */
export interface TransportTraceEventData {
    /**
     * Trace type.
     */
    type: TransportTraceEventType;
    /**
     * Event timestamp.
     */
    timestamp: number;
    /**
     * Event direction.
     */
    direction: 'in' | 'out';
    /**
     * Per type information.
     */
    info: any;
}
export declare type SctpState = 'new' | 'connecting' | 'connected' | 'failed' | 'closed';
export declare type TransportEvents = {
    routerclose: [];
    listenserverclose: [];
    trace: [TransportTraceEventData];
    '@close': [];
    '@newproducer': [Producer];
    '@producerclose': [Producer];
    '@newdataproducer': [DataProducer];
    '@dataproducerclose': [DataProducer];
    '@listenserverclose': [];
};
export declare type TransportObserverEvents = {
    close: [];
    newproducer: [Producer];
    newconsumer: [Consumer];
    newdataproducer: [DataProducer];
    newdataconsumer: [DataConsumer];
    trace: [TransportTraceEventData];
};
export declare type TransportConstructorOptions = {
    internal: TransportInternal;
    data: TransportData;
    channel: Channel;
    payloadChannel: PayloadChannel;
    appData?: Record<string, unknown>;
    getRouterRtpCapabilities: () => RtpCapabilities;
    getProducerById: (producerId: string) => Producer | undefined;
    getDataProducerById: (dataProducerId: string) => DataProducer | undefined;
};
export declare type TransportInternal = RouterInternal & {
    transportId: string;
};
declare type TransportData = WebRtcTransportData | PlainTransportData | PipeTransportData | DirectTransportData;
export declare class Transport<Events extends TransportEvents = TransportEvents, ObserverEvents extends TransportObserverEvents = TransportObserverEvents> extends EnhancedEventEmitter<Events> {
    #private;
    protected readonly internal: TransportInternal;
    protected readonly channel: Channel;
    protected readonly payloadChannel: PayloadChannel;
    protected readonly getProducerById: (producerId: string) => Producer | undefined;
    protected readonly getDataProducerById: (dataProducerId: string) => DataProducer | undefined;
    protected readonly consumers: Map<string, Consumer>;
    protected readonly dataProducers: Map<string, DataProducer>;
    protected readonly dataConsumers: Map<string, DataConsumer>;
    /**
     * @private
     * @interface
     */
    constructor({ internal, data, channel, payloadChannel, appData, getRouterRtpCapabilities, getProducerById, getDataProducerById }: TransportConstructorOptions);
    /**
     * Transport id.
     */
    get id(): string;
    /**
     * Whether the Transport is closed.
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
    get observer(): EnhancedEventEmitter<ObserverEvents>;
    /**
     * @private
     * Just for testing purposes.
     */
    get channelForTesting(): Channel;
    /**
     * Close the Transport.
     */
    close(): void;
    /**
     * Router was closed.
     *
     * @private
     * @virtual
     */
    routerClosed(): void;
    /**
     * Listen server was closed (this just happens in WebRtcTransports when their
     * associated WebRtcServer is closed).
     *
     * @private
     */
    listenServerClosed(): void;
    /**
     * Dump Transport.
     */
    dump(): Promise<any>;
    /**
     * Get Transport stats.
     *
     * @abstract
     */
    getStats(): Promise<any[]>;
    /**
     * Provide the Transport remote parameters.
     *
     * @abstract
     */
    connect(params: any): Promise<void>;
    /**
     * Set maximum incoming bitrate for receiving media.
     */
    setMaxIncomingBitrate(bitrate: number): Promise<void>;
    /**
     * Set maximum outgoing bitrate for sending media.
     */
    setMaxOutgoingBitrate(bitrate: number): Promise<void>;
    /**
     * Create a Producer.
     */
    produce({ id, kind, rtpParameters, paused, keyFrameRequestDelay, appData }: ProducerOptions): Promise<Producer>;
    /**
     * Create a Consumer.
     *
     * @virtual
     */
    consume({ producerId, rtpCapabilities, paused, mid, preferredLayers, ignoreDtx, pipe, appData }: ConsumerOptions): Promise<Consumer>;
    /**
     * Create a DataProducer.
     */
    produceData({ id, sctpStreamParameters, label, protocol, appData }?: DataProducerOptions): Promise<DataProducer>;
    /**
     * Create a DataConsumer.
     */
    consumeData({ dataProducerId, ordered, maxPacketLifeTime, maxRetransmits, appData }: DataConsumerOptions): Promise<DataConsumer>;
    /**
     * Enable 'trace' event.
     */
    enableTraceEvent(types?: TransportTraceEventType[]): Promise<void>;
    private getNextSctpStreamId;
}
export {};
//# sourceMappingURL=Transport.d.ts.map