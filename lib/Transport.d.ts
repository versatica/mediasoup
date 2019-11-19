import EnhancedEventEmitter from './EnhancedEventEmitter';
import Channel from './Channel';
import Producer, { ProducerOptions } from './Producer';
import Consumer, { ConsumerOptions } from './Consumer';
import DataProducer, { DataProducerOptions } from './DataProducer';
import DataConsumer, { DataConsumerOptions } from './DataConsumer';
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
 * Valid types for 'packet' event.
 */
export declare type TransportPacketEventType = 'probation' | 'bwe';
/**
 * 'packet' event data.
 */
export interface TransportPacketEventData {
    /**
     * Type of packet.
     */
    type: TransportPacketEventType;
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
export default class Transport extends EnhancedEventEmitter {
    protected readonly _internal: any;
    protected _data: any;
    protected readonly _channel: Channel;
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
    private _sctpStreamIds?;
    private _nextSctpStreamId;
    protected readonly _observer: EnhancedEventEmitter;
    /**
     * @private
     * @interface
     * @emits routerclose
     * @emits @close
     * @emits @newproducer
     * @emits @producerclose
     * @emits @newdataproducer
     * @emits @dataproducerclose
     */
    constructor({ internal, data, channel, appData, getRouterRtpCapabilities, getProducerById, getDataProducerById }: {
        internal: any;
        data: any;
        channel: Channel;
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
     * @emits {producer: Producer} newproducer
     * @emits {consumer: Consumer} newconsumer
     * @emits {producer: DataProducer} newdataproducer
     * @emits {consumer: DataConsumer} newdataconsumer
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
    getStats(): Promise<any>;
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
    produce({ id, kind, rtpParameters, paused, appData }: ProducerOptions): Promise<Producer>;
    /**
     * Create a Consumer.
     *
     * @virtual
     */
    consume({ producerId, rtpCapabilities, paused, preferredLayers, appData }: ConsumerOptions): Promise<Consumer>;
    /**
     * Create a DataProducer.
     */
    produceData({ id, sctpStreamParameters, label, protocol, appData }: DataProducerOptions): Promise<DataProducer>;
    /**
     * Create a DataConsumer.
     */
    consumeData({ dataProducerId, appData }: DataConsumerOptions): Promise<DataConsumer>;
    /**
     * Enable 'packet' event.
     */
    enablePacketEvent(types?: TransportPacketEventType[]): Promise<void>;
    private _getNextSctpStreamId;
}
//# sourceMappingURL=Transport.d.ts.map