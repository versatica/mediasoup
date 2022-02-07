import { Logger } from './Logger';
import { RtpObserver, RtpObserverEvents } from './RtpObserver';
import { Producer } from './Producer';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';

export interface ActiveSpeakerObserverOptions {
	interval?: number;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

export interface ActiveSpeakerObserverActivity {
	/**
	 * The producer instance.
	 */
	producer: Producer;
}

type ObserverEvents = RtpObserverEvents & {
	dominantspeaker: [{ producer: Producer }];
}

type Events = {
	routerclose: [];
} & Pick<ObserverEvents, 'dominantspeaker'>

const logger = new Logger('ActiveSpeakerObserver');

export class ActiveSpeakerObserver extends RtpObserver<Events>
{
	/**
	 * @private
	 */
	constructor(params: any)
	{
		super(params);

		this.handleWorkerNotifications();
	}

	/**
	 * Observer.
	 */
	get observer(): EnhancedEventEmitter<ObserverEvents>
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
					const dominantSpeaker = {
						producer : this.getProducerById(data.producerId)
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
