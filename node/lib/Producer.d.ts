/// <reference types="node" />
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { MediaKind, RtpParameters } from './RtpParameters';
export declare type ProducerOptions = {
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
     * Just for video. Time (in ms) before asking the sender for a new key frame
     * after having asked a previous one. Default 0.
     */
    keyFrameRequestDelay?: number;
    /**
     * Custom application data.
     */
    appData?: any;
};
/**
 * Valid types for 'trace' event.
 */
export declare type ProducerTraceEventType = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';
/**
 * 'trace' event data.
 */
export declare type ProducerTraceEventData = {
    /**
     * Trace type.
     */
    type: ProducerTraceEventType;
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
};
export declare type ProducerScore = {
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
};
export declare type ProducerVideoOrientation = {
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
};
export declare type ProducerStat = {
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
    rtxPacketsDiscarded?: number;
    jitter: number;
    bitrateByLayer?: any;
};
/**
 * Producer type.
 */
export declare type ProducerType = 'simple' | 'simulcast' | 'svc';
export declare type ProducerEvents = {
    transportclose: [];
    score: [ProducerScore[]];
    videoorientationchange: [ProducerVideoOrientation];
    trace: [ProducerTraceEventData];
};
export declare type ProducerObserverEvents = {
    close: [];
    pause: [];
    resume: [];
    score: [ProducerScore[]];
    videoorientationchange: [ProducerVideoOrientation];
    trace: [ProducerTraceEventData];
};
export declare class Producer extends EnhancedEventEmitter<ProducerEvents> {
    #private;
    /**
     * @private
     * @emits transportclose
     * @emits score - (score: ProducerScore[])
     * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
     * @emits trace - (trace: ProducerTraceEventData)
     * @emits @close
     */
    constructor({ internal, data, channel, payloadChannel, appData, paused }: {
        internal: any;
        data: any;
        channel: Channel;
        payloadChannel: PayloadChannel;
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
     * @emits score - (score: ProducerScore[])
     * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
     * @emits trace - (trace: ProducerTraceEventData)
     */
    get observer(): EnhancedEventEmitter<ProducerObserverEvents>;
    /**
     * @private
     * Just for testing purposes.
     */
    get channelForTesting(): Channel;
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
     * Enable 'trace' event.
     */
    enableTraceEvent(types?: ProducerTraceEventType[]): Promise<void>;
    /**
     * Send RTP packet (just valid for Producers created on a DirectTransport).
     */
    send(rtpPacket: Buffer): void;
    private handleWorkerNotifications;
}
//# sourceMappingURL=Producer.d.ts.map