import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import { MediaKind, RtpParameters } from './RtpParameters';

export type ProducerOptions =
{
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
}

/**
 * Valid types for 'trace' event.
 */
export type ProducerTraceEventType = 'rtp' | 'keyframe' | 'nack' | 'pli' | 'fir';

/**
 * 'trace' event data.
 */
export type ProducerTraceEventData =
{
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
}

export type ProducerScore =
{
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

export type ProducerVideoOrientation =
{
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

export type ProducerStat =
{
	// Common to all RtpStreams.
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
	// RtpStreamRecv specific.
	jitter: number;
	bitrateByLayer?: any;
}

/**
 * Producer type.
 */
export type ProducerType = 'simple' | 'simulcast' | 'svc';

const logger = new Logger('Producer');

export class Producer extends EnhancedEventEmitter
{
	// Internal data.
	private readonly _internal:
	{
		routerId: string;
		transportId: string;
		producerId: string;
	};

	// Producer data.
	private readonly _data:
	{
		kind: MediaKind;
		rtpParameters: RtpParameters;
		type: ProducerType;
		consumableRtpParameters: RtpParameters;
	};

	// Channel instance.
	private readonly _channel: Channel;

	// PayloadChannel instance.
	private readonly _payloadChannel: PayloadChannel;

	// Closed flag.
	private _closed = false;

	// Custom app data.
	private readonly _appData?: any;

	// Paused flag.
	private _paused = false;

	// Current score.
	private _score: ProducerScore[] = [];

	// Observer instance.
	private readonly _observer = new EnhancedEventEmitter();

	/**
	 * @private
	 * @emits transportclose
	 * @emits score - (score: ProducerScore[])
	 * @emits videoorientationchange - (videoOrientation: ProducerVideoOrientation)
	 * @emits trace - (trace: ProducerTraceEventData)
	 * @emits @close
	 */
	constructor(
		{
			internal,
			data,
			channel,
			payloadChannel,
			appData,
			paused
		}:
		{
			internal: any;
			data: any;
			channel: Channel;
			payloadChannel: PayloadChannel;
			appData?: any;
			paused: boolean;
		}
	)
	{
		super();

		logger.debug('constructor()');

		this._internal = internal;
		this._data = data;
		this._channel = channel;
		this._payloadChannel = payloadChannel;
		this._appData = appData;
		this._paused = paused;

		this._handleWorkerNotifications();
	}

	/**
	 * Producer id.
	 */
	get id(): string
	{
		return this._internal.producerId;
	}

	/**
	 * Whether the Producer is closed.
	 */
	get closed(): boolean
	{
		return this._closed;
	}

	/**
	 * Media kind.
	 */
	get kind(): MediaKind
	{
		return this._data.kind;
	}

	/**
	 * RTP parameters.
	 */
	get rtpParameters(): RtpParameters
	{
		return this._data.rtpParameters;
	}

	/**
	 * Producer type.
	 */
	get type(): ProducerType
	{
		return this._data.type;
	}

	/**
	 * Consumable RTP parameters.
	 *
	 * @private
	 */
	get consumableRtpParameters(): RtpParameters
	{
		return this._data.consumableRtpParameters;
	}

	/**
	 * Whether the Producer is paused.
	 */
	get paused(): boolean
	{
		return this._paused;
	}

	/**
	 * Producer score list.
	 */
	get score(): ProducerScore[]
	{
		return this._score;
	}

	/**
	 * App custom data.
	 */
	get appData(): any
	{
		return this._appData;
	}

	/**
	 * Invalid setter.
	 */
	set appData(appData: any) // eslint-disable-line no-unused-vars
	{
		throw new Error('cannot override appData object');
	}

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
	get observer(): EnhancedEventEmitter
	{
		return this._observer;
	}

	/**
	 * Close the Producer.
	 */
	close(): void
	{
		if (this._closed)
			return;

		logger.debug('close()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.producerId);
		this._payloadChannel.removeAllListeners(this._internal.producerId);

		this._channel.request('producer.close', this._internal)
			.catch(() => {});

		this.emit('@close');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	transportClosed(): void
	{
		if (this._closed)
			return;

		logger.debug('transportClosed()');

		this._closed = true;

		// Remove notification subscriptions.
		this._channel.removeAllListeners(this._internal.producerId);
		this._payloadChannel.removeAllListeners(this._internal.producerId);

		this.safeEmit('transportclose');

		// Emit observer event.
		this._observer.safeEmit('close');
	}

	/**
	 * Dump Producer.
	 */
	async dump(): Promise<any>
	{
		logger.debug('dump()');

		return this._channel.request('producer.dump', this._internal);
	}

	/**
	 * Get Producer stats.
	 */
	async getStats(): Promise<ProducerStat[]>
	{
		logger.debug('getStats()');

		return this._channel.request('producer.getStats', this._internal);
	}

	/**
	 * Pause the Producer.
	 */
	async pause(): Promise<void>
	{
		logger.debug('pause()');

		const wasPaused = this._paused;

		await this._channel.request('producer.pause', this._internal);

		this._paused = true;

		// Emit observer event.
		if (!wasPaused)
			this._observer.safeEmit('pause');
	}

	/**
	 * Resume the Producer.
	 */
	async resume(): Promise<void>
	{
		logger.debug('resume()');

		const wasPaused = this._paused;

		await this._channel.request('producer.resume', this._internal);

		this._paused = false;

		// Emit observer event.
		if (wasPaused)
			this._observer.safeEmit('resume');
	}

	/**
	 * Enable 'trace' event.
	 */
	async enableTraceEvent(types: ProducerTraceEventType[] = []): Promise<void>
	{
		logger.debug('enableTraceEvent()');

		const reqData = { types };

		await this._channel.request(
			'producer.enableTraceEvent', this._internal, reqData);
	}

	/**
	 * Send RTP packet (just valid for Producers created on a DirectTransport).
	 */
	send(rtpPacket: Buffer)
	{
		if (!Buffer.isBuffer(rtpPacket))
		{
			throw new TypeError('rtpPacket must be a Buffer');
		}

		this._payloadChannel.notify(
			'producer.send', this._internal, undefined, rtpPacket);
	}

	private _handleWorkerNotifications(): void
	{
		this._channel.on(this._internal.producerId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case 'score':
				{
					const score = data as ProducerScore[];

					this._score = score;

					this.safeEmit('score', score);

					// Emit observer event.
					this._observer.safeEmit('score', score);

					break;
				}

				case 'videoorientationchange':
				{
					const videoOrientation = data as ProducerVideoOrientation;

					this.safeEmit('videoorientationchange', videoOrientation);

					// Emit observer event.
					this._observer.safeEmit('videoorientationchange', videoOrientation);

					break;
				}

				case 'trace':
				{
					const trace = data as ProducerTraceEventData;

					this.safeEmit('trace', trace);

					// Emit observer event.
					this._observer.safeEmit('trace', trace);

					break;
				}

				default:
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});
	}
}
