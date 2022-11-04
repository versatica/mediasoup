import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import {
	RtpObserver,
	RtpObserverEvents,
	RtpObserverObserverEvents,
	RtpObserverConstructorOptions
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
	dominantspeaker: [Producer];
};

export type ActiveSpeakerObserverObserverEvents = RtpObserverObserverEvents &
{
	dominantspeaker: [Producer];
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

					this.safeEmit('dominantspeaker', producer);
					this.observer.safeEmit('dominantspeaker', producer);

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
