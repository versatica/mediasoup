import { Logger } from './Logger';
import { RtpObserver } from './RtpObserver';
import { Producer } from './Producer';

export interface AudioLevelObserverOptions
{
	/**
	 * Maximum number of entries in the 'volumes”' event. Default 1.
	 */
	maxEntries?: number;

	/**
	 * Minimum average volume (in dBvo from -127 to 0) for entries in the
	 * 'volumes' event.	Default -80.
	 */
	threshold?: number;

	/**
	 * Interval in ms for checking audio volumes. Default 1000.
	 */
	interval?: number;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

export interface AudioLevelObserverVolume
{
	/**
	 * The audio producer instance.
	 */
	producer: Producer;

	/**
	 * The average volume (in dBvo from -127 to 0) of the audio producer in the
	 * last interval.
	 */
	volume: number;
}

const logger = new Logger('AudioLevelObserver');

export class AudioLevelObserver extends RtpObserver
{
	/**
	 * @private
	 * @emits volumes - (volumes: AudioLevelObserverVolume[])
	 * @emits silence
	 */
	constructor(params: any)
	{
		super(params);

		this.handleWorkerNotifications();
	}

	/**
	 * Observer.
	 *
	 * @emits close
	 * @emits pause
	 * @emits resume
	 * @emits addproducer - (producer: Producer)
	 * @emits removeproducer - (producer: Producer)
	 * @emits volumes - (volumes: AudioLevelObserverVolume[])
	 * @emits silence
	 */
	// get observer(): EnhancedEventEmitter

	private handleWorkerNotifications(): void
	{
		this.channel.on(this.internal.rtpObserverId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case 'volumes':
				{
					// Get the corresponding Producer instance and remove entries with
					// no Producer (it may have been closed in the meanwhile).
					const volumes: AudioLevelObserverVolume[] = data
						.map(({ producerId, volume }: { producerId: string; volume: number }) => (
							{
								producer : this.getProducerById(producerId),
								volume
							}
						))
						.filter(({ producer }: { producer: Producer }) => producer);

					if (volumes.length > 0)
					{
						this.safeEmit('volumes', volumes);

						// Emit observer event.
						this.observer.safeEmit('volumes', volumes);
					}

					break;
				}

				case 'silence':
				{
					this.safeEmit('silence');

					// Emit observer event.
					this.observer.safeEmit('silence');

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
