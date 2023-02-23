import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { TransportInternal } from './Transport';
import { MediaKind, RtpParameters, parseRtpParameters } from './RtpParameters';
import { Event, Notification } from './fbs/notification_generated';
import { parseRtpStreamRecvStats, RtpStreamRecvStats } from './RtpStream';
import * as FbsNotification from './fbs/notification_generated';
import * as FbsRequest from './fbs/request_generated';
import * as FbsTransport from './fbs/transport_generated';
import * as FbsProducer from './fbs/producer_generated';
import * as utils from './utils';

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
	appData?: Record<string, unknown>;
};

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
};

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
};

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
};

export type ProducerStat = RtpStreamRecvStats;

/**
 * Producer type.
 */
export type ProducerType = 'simple' | 'simulcast' | 'svc';

export type ProducerEvents =
{
	transportclose: [];
	score: [ProducerScore[]];
	videoorientationchange: [ProducerVideoOrientation];
	trace: [ProducerTraceEventData];
	// Private events.
	'@close': [];
};

export type ProducerObserverEvents =
{
	close: [];
	pause: [];
	resume: [];
	score: [ProducerScore[]];
	videoorientationchange: [ProducerVideoOrientation];
	trace: [ProducerTraceEventData];
};

type ProducerDump = {
	id: string;
	kind: string;
	type:ProducerType;
	rtpParameters:RtpParameters;
	rtpMapping:any;
	rtpStreams:any;
	traceEventTypes:string[];
	paused:boolean;
};

type ProducerInternal = TransportInternal &
{
	producerId: string;
};

type ProducerData =
{
	kind: MediaKind;
	rtpParameters: RtpParameters;
	type: ProducerType;
	consumableRtpParameters: RtpParameters;
};

const logger = new Logger('Producer');

export class Producer extends EnhancedEventEmitter<ProducerEvents>
{
	// Internal data.
	readonly #internal: ProducerInternal;

	// Producer data.
	readonly #data: ProducerData;

	// Channel instance.
	readonly #channel: Channel;

	// Closed flag.
	#closed = false;

	// Custom app data.
	readonly #appData: Record<string, unknown>;

	// Paused flag.
	#paused = false;

	// Current score.
	#score: ProducerScore[] = [];

	// Observer instance.
	readonly #observer = new EnhancedEventEmitter<ProducerObserverEvents>();

	/**
	 * @private
	 */
	constructor(
		{
			internal,
			data,
			channel,
			appData,
			paused
		}:
		{
			internal: ProducerInternal;
			data: ProducerData;
			channel: Channel;
			appData?: Record<string, unknown>;
			paused: boolean;
		}
	)
	{
		super();

		logger.debug('constructor()');

		this.#internal = internal;
		this.#data = data;
		this.#channel = channel;
		this.#appData = appData || {};
		this.#paused = paused;

		this.handleWorkerNotifications();
	}

	/**
	 * Producer id.
	 */
	get id(): string
	{
		return this.#internal.producerId;
	}

	/**
	 * Whether the Producer is closed.
	 */
	get closed(): boolean
	{
		return this.#closed;
	}

	/**
	 * Media kind.
	 */
	get kind(): MediaKind
	{
		return this.#data.kind;
	}

	/**
	 * RTP parameters.
	 */
	get rtpParameters(): RtpParameters
	{
		return this.#data.rtpParameters;
	}

	/**
	 * Producer type.
	 */
	get type(): ProducerType
	{
		return this.#data.type;
	}

	/**
	 * Consumable RTP parameters.
	 *
	 * @private
	 */
	get consumableRtpParameters(): RtpParameters
	{
		return this.#data.consumableRtpParameters;
	}

	/**
	 * Whether the Producer is paused.
	 */
	get paused(): boolean
	{
		return this.#paused;
	}

	/**
	 * Producer score list.
	 */
	get score(): ProducerScore[]
	{
		return this.#score;
	}

	/**
	 * App custom data.
	 */
	get appData(): Record<string, unknown>
	{
		return this.#appData;
	}

	/**
	 * Invalid setter.
	 */
	set appData(appData: Record<string, unknown>) // eslint-disable-line no-unused-vars
	{
		throw new Error('cannot override appData object');
	}

	/**
	 * Observer.
	 */
	get observer(): EnhancedEventEmitter<ProducerObserverEvents>
	{
		return this.#observer;
	}

	/**
	 * @private
	 * Just for testing purposes.
	 */
	get channelForTesting(): Channel
	{
		return this.#channel;
	}

	/**
	 * Close the Producer.
	 */
	close(): void
	{
		if (this.#closed)
		{
			return;
		}

		logger.debug('close()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.#channel.removeAllListeners(this.#internal.producerId);

		/* Build Request. */

		const requestOffset = new FbsTransport.CloseProducerRequestT(
			this.#internal.producerId
		).pack(this.#channel.bufferBuilder);

		this.#channel.request(
			FbsRequest.Method.TRANSPORT_CLOSE_PRODUCER,
			FbsRequest.Body.FBS_Transport_CloseProducerRequest,
			requestOffset,
			this.#internal.transportId
		).catch(() => {});

		this.emit('@close');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Transport was closed.
	 *
	 * @private
	 */
	transportClosed(): void
	{
		if (this.#closed)
		{
			return;
		}

		logger.debug('transportClosed()');

		this.#closed = true;

		// Remove notification subscriptions.
		this.#channel.removeAllListeners(this.#internal.producerId);

		this.safeEmit('transportclose');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	/**
	 * Dump Producer.
	 */
	async dump(): Promise<ProducerDump>
	{
		logger.debug('dump()');

		const response = await this.#channel.request(
			FbsRequest.Method.PRODUCER_DUMP,
			undefined,
			undefined,
			this.#internal.producerId
		);

		/* Decode the response. */
		const dumpResponse = new FbsProducer.DumpResponse();

		response.body(dumpResponse);

		return parseProducerDump(dumpResponse);
	}

	/**
	 * Get Producer stats.
	 */
	async getStats(): Promise<ProducerStat[]>
	{
		logger.debug('getStats()');

		const response = await this.#channel.request(
			FbsRequest.Method.PRODUCER_GET_STATS,
			undefined,
			undefined,
			this.#internal.producerId
		);

		/* Decode the response. */
		const data = new FbsProducer.GetStatsResponse();

		response.body(data);

		return parseProducerStats(data);
	}

	/**
	 * Pause the Producer.
	 */
	async pause(): Promise<void>
	{
		logger.debug('pause()');

		const wasPaused = this.#paused;

		await this.#channel.request(
			FbsRequest.Method.PRODUCER_PAUSE,
			undefined,
			undefined,
			this.#internal.producerId
		);

		this.#paused = true;

		// Emit observer event.
		if (!wasPaused)
		{
			this.#observer.safeEmit('pause');
		}
	}

	/**
	 * Resume the Producer.
	 */
	async resume(): Promise<void>
	{
		logger.debug('resume()');

		const wasPaused = this.#paused;

		await this.#channel.request(
			FbsRequest.Method.PRODUCER_RESUME,
			undefined,
			undefined,
			this.#internal.producerId
		);

		this.#paused = false;

		// Emit observer event.
		if (wasPaused)
		{
			this.#observer.safeEmit('resume');
		}
	}

	/**
	 * Enable 'trace' event.
	 */
	async enableTraceEvent(types: ProducerTraceEventType[] = []): Promise<void>
	{
		logger.debug('enableTraceEvent()');

		if (!Array.isArray(types))
		{
			throw new TypeError('types must be an array');
		}
		if (types.find((type) => typeof type !== 'string'))
		{
			throw new TypeError('every type must be a string');
		}

		/* Build Request. */

		const requestOffset = new FbsProducer.EnableTraceEventRequestT(
			types
		).pack(this.#channel.bufferBuilder);

		await this.#channel.request(
			FbsRequest.Method.PRODUCER_ENABLE_TRACE_EVENT,
			FbsRequest.Body.FBS_Producer_EnableTraceEventRequest,
			requestOffset,
			this.#internal.producerId
		);
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

		const builder = this.#channel.bufferBuilder;
		const dataOffset = FbsProducer.SendNotification.createDataVector(builder, rtpPacket);
		const notificationOffset = FbsProducer.SendNotification.createSendNotification(
			builder,
			dataOffset
		);

		this.#channel.notify(
			FbsNotification.Event.PRODUCER_SEND,
			FbsNotification.Body.FBS_Producer_SendNotification,
			notificationOffset,
			this.#internal.producerId
		);
	}

	private handleWorkerNotifications(): void
	{
		this.#channel.on(this.#internal.producerId, (event: Event, data?: Notification) =>
		{
			switch (event)
			{
				case Event.PRODUCER_SCORE:
				{
					const notification = new FbsProducer.ScoreNotification();

					data!.body(notification);

					const score: ProducerScore[] = utils.parseVector(
						notification, 'scores', parseProducerScore
					);

					this.#score = score;

					this.safeEmit('score', score);

					// Emit observer event.
					this.#observer.safeEmit('score', score);

					break;
				}

				case Event.PRODUCER_VIDEO_ORIENTATION_CHANGE:
				{
					const notification = new FbsProducer.VideoOrientationChangeNotification();

					data!.body(notification);

					const videoOrientation: ProducerVideoOrientation = notification.unpack();

					this.safeEmit('videoorientationchange', videoOrientation);

					// Emit observer event.
					this.#observer.safeEmit('videoorientationchange', videoOrientation);

					break;
				}

				case Event.PRODUCER_TRACE:
				{
					const notification = new FbsProducer.TraceNotification();

					data!.body(notification);

					const trace: ProducerTraceEventData = parseTraceEventData(notification);

					this.safeEmit('trace', trace);

					// Emit observer event.
					this.#observer.safeEmit('trace', trace);

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

export function parseProducerDump(
	data: FbsProducer.DumpResponse
): ProducerDump
{
	return {
		id            : data.id()!,
		kind          : data.kind() === FbsTransport.MediaKind.AUDIO ? 'audio' : 'video',
		type          : data.type()! as ProducerType,
		rtpParameters : parseRtpParameters(data.rtpParameters()!),
		// NOTE: optional values are represented with null instead of undefined.
		// TODO: Make flatbuffers TS return undefined instead of null.
		rtpMapping    : data.rtpMapping() ? data.rtpMapping()!.unpack() : undefined,
		// NOTE: optional values are represented with null instead of undefined.
		// TODO: Make flatbuffers TS return undefined instead of null.
		rtpStreams    : data.rtpStreamsLength() > 0 ?
			utils.parseVector(data, 'rtpStreams', (rtpStream: any) => rtpStream.unpack()) :
			undefined,
		traceEventTypes : utils.parseVector(data, 'traceEventTypes'),
		paused          : data.paused()
	};
}

function parseProducerStats(binary: FbsProducer.GetStatsResponse): ProducerStat[]
{
	return utils.parseVector(binary, 'stats', parseRtpStreamRecvStats);
}

function parseProducerScore(
	binary: FbsProducer.Score
): ProducerScore
{
	return {
		ssrc  : binary.score(),
		rid   : binary.rid() ?? undefined,
		score : binary.score()
	};
}

function parseTraceEventData(
	trace: FbsProducer.TraceNotification
): ProducerTraceEventData
{
	let info: any;

	if (trace.infoType() !== FbsProducer.TraceInfo.NONE)
	{
		const accessor = trace.info.bind(trace);

		info = FbsProducer.unionToTraceInfo(trace.infoType(), accessor);

		trace.info(info);
	}

	return {
		type      : fbstraceType2String(trace.type()),
		timestamp : Number(trace.timestamp()),
		direction : trace.direction() === FbsProducer.TraceDirection.DIRECTION_IN ? 'in' : 'out',
		info      : info ? info.unpack() : undefined
	};
}

function fbstraceType2String(traceType: FbsProducer.TraceType): ProducerTraceEventType
{
	switch (traceType)
	{
		case FbsProducer.TraceType.KEYFRAME:
			return 'keyframe';
		case FbsProducer.TraceType.FIR:
			return 'fir';
		case FbsProducer.TraceType.NACK:
			return 'nack';
		case FbsProducer.TraceType.PLI:
			return 'pli';
		case FbsProducer.TraceType.RTP:
			return 'rtp';
		default:
			throw new TypeError(`invalid TraceType: ${traceType}`);
	}
}
