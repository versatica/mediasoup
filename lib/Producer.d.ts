import EnhancedEventEmitter from './EnhancedEventEmitter';
import Channel from './Channel';
import { MediaKind, RtpParameters } from './RtpParameters';
export interface ProducerOptions {
    /**
     * Producer id (just for Router.pipeToRouter() method).
     */
    id?: string;
    /**
     * Media kind ('audio' or 'video').
     */
    kind: MediaKind;
    /**
     * RTP parameters defining what the endpoint is sending.
     */
    rtpParameters: RtpParameters;
    /**
     * Whether the producer must start in paused mode. Default false.
     */
    paused?: boolean;
    /**
     * Custom application data.
     */
    appData?: any;
}
/**
 * Valid types for 'packet' event.
 */
export declare type ProducerPacketEventType = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
/**
 * 'packet' event data.
 */
export interface ProducerPacketEventData {
    /**
     * Type of packet.
     */
    type: ProducerPacketEventType;
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
export interface ProducerScore {
    /**
     * SSRC of the RTP stream.
     */
    ssrc: number;
    /**
     * RID of the RTP stream.
     */
    rid?: string;
    /**
     * The score of the RTP stream.
     */
    score: number;
}
export interface ProducerVideoOrientation {
    /**
     * Whether the source is a video camera.
     */
    camera: boolean;
    /**
     * Whether the video source is flipped.
     */
    flip: boolean;
    /**
     * Rotation degrees (0, 90, 180 or 270).
     */
    rotation: number;
}
export interface ProducerStat {
    type: string;
    timestamp: number;
    ssrc: number;
    rtxSsrc?: number;
    rid?: string;
    kind: string;
    mimeType: string;
    packetsLost: number;
    fractionLost: number;
    packetsDiscarded: number;
    packetsRetransmitted: number;
    packetsRepaired: number;
    nackCount: number;
    nackPacketCount: number;
    pliCount: number;
    firCount: number;
    score: number;
    packetCount: number;
    byteCount: number;
    bitrate: number;
    roundTripTime?: number;
    jitter: number;
    bitrateByLayer?: any;
}
/**
 * Producer type.
 */
export declare type ProducerType = 'simple' | 'simulcast' | 'svc';
export default class Producer extends EnhancedEventEmitter {
    private readonly _internal;
    private readonly _data;
    private readonly _channel;
    private _closed;
    private readonly _appData?;
    private _paused;
    private _score;
    private readonly _observer;
    /**
     * @private
     * @emits transportclose
     * @emits {ProducerScore[]} score
     * @emits {ProducerVideoOrientation} videoorientationchange
     * @emits {ProducerPacketEventData} packet
     * @emits @close
     */
    constructor({ internal, data, channel, appData, paused }: {
        internal: any;
        data: any;
        channel: Channel;
        appData?: any;
        paused: boolean;
    });
    /**
     * Producer id.
     */
    get id(): string;
    /**
     * Whether the Producer is closed.
     */
    get closed(): boolean;
    /**
     * Media kind.
     */
    get kind(): MediaKind;
    /**
     * RTP parameters.
     */
    get rtpParameters(): RtpParameters;
    /**
     * Producer type.
     */
    get type(): ProducerType;
    /**
     * Consumable RTP parameters.
     *
     * @private
     */
    get consumableRtpParameters(): RtpParameters;
    /**
     * Whether the Producer is paused.
     */
    get paused(): boolean;
    /**
     * Producer score list.
     */
    get score(): ProducerScore[];
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
     * @emits pause
     * @emits resume
     * @emits {ProducerScore[]} score
     * @emits {ProducerVideoOrientation} videoorientationchange
     * @emits {ProducerPacketEventData} packet
     */
    get observer(): EnhancedEventEmitter;
    /**
     * Close the Producer.
     */
    close(): void;
    /**
     * Transport was closed.
     *
     * @private
     */
    transportClosed(): void;
    /**
     * Dump Producer.
     */
    dump(): Promise<any>;
    /**
     * Get Producer stats.
     */
    getStats(): Promise<ProducerStat[]>;
    /**
     * Pause the Producer.
     */
    pause(): Promise<void>;
    /**
     * Resume the Producer.
     */
    resume(): Promise<void>;
    /**
     * Enable 'packet' event.
     */
    enablePacketEvent(types?: ProducerPacketEventType[]): Promise<void>;
    private _handleWorkerNotifications;
}
//# sourceMappingURL=Producer.d.ts.map