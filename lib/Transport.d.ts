import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { Producer, ProducerOptions } from './Producer';
import { Consumer, ConsumerOptions } from './Consumer';
import { DataProducer, DataProducerOptions } from './DataProducer';
import { DataConsumer, DataConsumerOptions } from './DataConsumer';
import { RtpCapabilities } from './RtpParameters';
import { SctpParameters } from './SctpParameters';
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
export declare class Transport extends EnhancedEventEmitter {
    protected readonly _internal: {
        routerId: string;
        transportId: string;
    };
    protected readonly _data: {
        sctpParameters?: SctpParameters;
        sctpState?: SctpState;
    };
    protected readonly _channel: Channel;
    protected readonly _payloadChannel: PayloadChannel;
    protected _closed: boolean;
    private readonly _appData?;
    protected readonly _getRouterRtpCapabilities: () => RtpCapabilities;
    protected readonly _getProducerById: (producerId: string) => Producer;
    protected readonly _getDataProducerById: (dataProducerId: string) => DataProducer;
    protected readonly _producers: Map<string, Producer>;
    protected readonly _consumers: Map<string, Consumer>;
    protected readonly _dataProducers: Map<string, DataProducer>;
    protected readonly _dataConsumers: Map<string, DataConsumer>;
    private _cnameForProducers?;
    private _nextMidForConsumers;
    private _sctpStreamIds?;
    private _nextSctpStreamId;
    protected readonly _observer: EnhancedEventEmitter;
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     * @emits @newproducer - (producer: Producer)
     * @emits @producerclose - (producer: Producer)
     * @emits @newdataproducer - (dataProducer: DataProducer)
     * @emits @dataproducerclose - (dataProducer: DataProducer)
     */
    constructor({ internal, data, channel, payloadChannel, appData, getRouterRtpCapabilities, getProducerById, getDataProducerById }: {
        internal: any;
        data: any;
        channel: Channel;
        payloadChannel: PayloadChannel;
        appData: any;
        getRouterRtpCapabilities: () => RtpCapabilities;
        getProducerById: (producerId: string) => Producer;
        getDataProducerById: (dataProducerId: string) => DataProducer;
    });
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
    get appData(): any;
    /**
     * Invalid setter.
     */
    set appData(appData: any);
    /**
     * Observer.
     *
     * @emits close
     * @emits newproducer - (producer: Producer)
     * @emits newconsumer - (producer: Producer)
     * @emits newdataproducer - (dataProducer: DataProducer)
     * @emits newdataconsumer - (dataProducer: DataProducer)
     */
    get observer(): EnhancedEventEmitter;
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
     * Create a Producer.
     */
    produce({ id, kind, rtpParameters, paused, keyFrameRequestDelay, appData }: ProducerOptions): Promise<Producer>;
    /**
     * Create a Consumer.
     *
     * @virtual
     */
    consume({ producerId, rtpCapabilities, paused, preferredLayers, pipe, appData }: ConsumerOptions): Promise<Consumer>;
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
    private _getNextSctpStreamId;
}
//# sourceMappingURL=Transport.d.ts.map