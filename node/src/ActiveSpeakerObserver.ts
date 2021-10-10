import { Logger } from './Logger';
import { RtpObserver } from './RtpObserver';
import { Producer } from './Producer';

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

const logger = new Logger('ActiveSpeakerObserver');

export class ActiveSpeakerObserver extends RtpObserver
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
	// get observer(): EnhancedEventEmitter

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
