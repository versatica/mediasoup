import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import {
	RtpObserver,
	RtpObserverEvents,
	RtpObserverObserverEvents,
	RtpObserverConstructorOptions
} from './RtpObserver';
import { Producer } from './Producer';
import { Event, Notification } from './fbs/notification_generated';
import * as FbsActiveSpeakerObserver from './fbs/activeSpeakerObserver_generated';

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
		this.channel.on(this.internal.rtpObserverId, (event: Event, data?: Notification) =>
		{
			switch (event)
			{
				case Event.ACTIVESPEAKEROBSERVER_DOMINANT_SPEAKER:
				{
					const notification = new FbsActiveSpeakerObserver.DominantSpeakerNotification();

					data!.body(notification);

					if (!notification.producerId())
						break;

					const producer = this.getProducerById(notification.producerId()!);

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
