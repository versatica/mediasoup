import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import {
	RtpObserver,
	RtpObserverEvents,
	RtpObserverObserverEvents,
	RtpObserverConstructorOptions
} from './RtpObserver';
import { Producer } from './Producer';

export type ActiveSpeakerObserverOptions =
{
	interval?: number;

	/**
	 * Custom application data.
	 */
	appData?: Record<string, unknown>;
};

export type ActiveSpeakerObserverDominantSpeaker =
{
	/**
	 * The audio Producer instance.
	 */
	producer: Producer;
};

export type ActiveSpeakerObserverEvents = RtpObserverEvents &
{
	dominantspeaker: [ActiveSpeakerObserverDominantSpeaker];
};

export type ActiveSpeakerObserverObserverEvents = RtpObserverObserverEvents &
{
	dominantspeaker: [ActiveSpeakerObserverDominantSpeaker];
};

type RtpObserverObserverConstructorOptions = RtpObserverConstructorOptions;

const logger = new Logger('ActiveSpeakerObserver');

export class ActiveSpeakerObserver extends RtpObserver<ActiveSpeakerObserverEvents>
{
	/**
	 * @private
	 */
	constructor(options: RtpObserverObserverConstructorOptions)
	{
		super(options);

		this.handleWorkerNotifications();
	}

	/**
	 * Observer.
	 */
	get observer(): EnhancedEventEmitter<ActiveSpeakerObserverObserverEvents>
	{
		return super.observer;
	}

	private handleWorkerNotifications(): void
	{
		this.channel.on(this.internal.rtpObserverId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case 'dominantspeaker':
				{
					const producer = this.getProducerById(data.producerId);

					if (!producer)
						break;

					const dominantSpeaker: ActiveSpeakerObserverDominantSpeaker =
					{
						producer
					};

					this.safeEmit('dominantspeaker', dominantSpeaker);
					this.observer.safeEmit('dominantspeaker', dominantSpeaker);

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
