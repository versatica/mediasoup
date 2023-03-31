import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import {
	RtpObserver,
	RtpObserverEvents,
	RtpObserverObserverEvents,
	RtpObserverConstructorOptions
} from './RtpObserver';
import { Producer } from './Producer';
import { AppData } from './types';

export type AudioLevelObserverOptions<AudioLevelObserverAppData extends AppData = AppData> =
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
	appData?: AudioLevelObserverAppData;
};

export type AudioLevelObserverVolume =
{
	/**
	 * The audio Producer instance.
	 */
	producer: Producer;

	/**
	 * The average volume (in dBvo from -127 to 0) of the audio Producer in the
	 * last interval.
	 */
	volume: number;
};

export type AudioLevelObserverEvents = RtpObserverEvents &
{
	volumes: [AudioLevelObserverVolume[]];
	silence: [];
};

export type AudioLevelObserverObserverEvents = RtpObserverObserverEvents & 
{
	volumes: [AudioLevelObserverVolume[]];
	silence: [];
};

type AudioLevelObserverConstructorOptions<AudioLevelObserverAppData> =
	RtpObserverConstructorOptions<AudioLevelObserverAppData>;

const logger = new Logger('AudioLevelObserver');

export class AudioLevelObserver<AudioLevelObserverAppData extends AppData = AppData>
	extends RtpObserver<AudioLevelObserverEvents, AudioLevelObserverAppData>
{
	/**
	 * @private
	 */
	constructor(options: AudioLevelObserverConstructorOptions<AudioLevelObserverAppData>)
	{
		super(options);

		this.handleWorkerNotifications();
	}

	/**
	 * Observer.
	 */
	get observer(): EnhancedEventEmitter<AudioLevelObserverObserverEvents>
	{
		return super.observer;
	}

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
