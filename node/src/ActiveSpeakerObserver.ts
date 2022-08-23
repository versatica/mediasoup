import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Channel } from './Channel';
import { PayloadChannel } from './PayloadChannel';
import {
	RtpObserver,
	RtpObserverEvents,
	RtpObserverObserverEvents,
	RtpObserverObserverInternal
} from './RtpObserver';
import { Producer } from './Producer';

export interface ActiveSpeakerObserverOptions 
{
	interval?: number;

	/**
	 * Custom application data.
	 */
	appData?: Record<string, unknown>;
}

export interface ActiveSpeakerObserverActivity 
{
	/**
	 * The producer instance.
	 */
	producer: Producer;
}

export type ActiveSpeakerObserverEvents = RtpObserverEvents &
{
	dominantspeaker: [{ producer: Producer }];
};

export type ActiveSpeakerObserverObserverEvents = RtpObserverObserverEvents &
{
	dominantspeaker: [{ producer: Producer }];
};

const logger = new Logger('ActiveSpeakerObserver');

export class ActiveSpeakerObserver extends RtpObserver<ActiveSpeakerObserverEvents>
{
	/**
	 * @private
	 */
	constructor(
		{
			internal,
			channel,
			payloadChannel,
			appData,
			getProducerById
		}:
		{
			internal: RtpObserverObserverInternal;
			channel: Channel;
			payloadChannel: PayloadChannel;
			appData?: Record<string, unknown>;
			getProducerById: (producerId: string) => Producer | undefined;
		}
	)
	{
		super(
			{
				internal,
				channel,
				payloadChannel,
				appData,
				getProducerById
			});

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

					const dominantSpeaker = { producer };

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
