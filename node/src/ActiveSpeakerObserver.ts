import { Logger } from './Logger';
import { EnhancedEventEmitter } from './enhancedEvents';
import {
	RtpObserver,
	RtpObserverEvents,
	RtpObserverObserverEvents,
	RtpObserverConstructorOptions,
} from './RtpObserver';
import { Producer } from './Producer';
import { AppData } from './types';
import { Event, Notification } from './fbs/notification';
import * as FbsActiveSpeakerObserver from './fbs/active-speaker-observer';

export type ActiveSpeakerObserverOptions<
	ActiveSpeakerObserverAppData extends AppData = AppData,
> = {
	interval?: number;

	/**
	 * Custom application data.
	 */
	appData?: ActiveSpeakerObserverAppData;
};

export type ActiveSpeakerObserverDominantSpeaker = {
	/**
	 * The audio Producer instance.
	 */
	producer: Producer;
};

export type ActiveSpeakerObserverEvents = RtpObserverEvents & {
	dominantspeaker: [ActiveSpeakerObserverDominantSpeaker];
};

export type ActiveSpeakerObserverObserverEvents = RtpObserverObserverEvents & {
	dominantspeaker: [ActiveSpeakerObserverDominantSpeaker];
};

type RtpObserverObserverConstructorOptions<ActiveSpeakerObserverAppData> =
	RtpObserverConstructorOptions<ActiveSpeakerObserverAppData>;

const logger = new Logger('ActiveSpeakerObserver');

export class ActiveSpeakerObserver<
	ActiveSpeakerObserverAppData extends AppData = AppData,
> extends RtpObserver<
	ActiveSpeakerObserverAppData,
	ActiveSpeakerObserverEvents
> {
	/**
	 * @private
	 */
	constructor(
		options: RtpObserverObserverConstructorOptions<ActiveSpeakerObserverAppData>
	) {
		super(options);

		this.handleWorkerNotifications();
	}

	/**
	 * Observer.
	 */
	get observer(): EnhancedEventEmitter<ActiveSpeakerObserverObserverEvents> {
		return super.observer;
	}

	private handleWorkerNotifications(): void {
		this.channel.on(
			this.internal.rtpObserverId,
			(event: Event, data?: Notification) => {
				switch (event) {
					case Event.ACTIVESPEAKEROBSERVER_DOMINANT_SPEAKER: {
						const notification =
							new FbsActiveSpeakerObserver.DominantSpeakerNotification();

						data!.body(notification);

						const producer = this.getProducerById(notification.producerId()!);

						if (!producer) {
							break;
						}

						const dominantSpeaker: ActiveSpeakerObserverDominantSpeaker = {
							producer,
						};

						this.safeEmit('dominantspeaker', dominantSpeaker);
						this.observer.safeEmit('dominantspeaker', dominantSpeaker);

						break;
					}

					default: {
						logger.error('ignoring unknown event "%s"', event);
					}
				}
			}
		);
	}
}
