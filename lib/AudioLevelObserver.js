const Logger = require('./Logger');
const RtpObserver = require('./RtpObserver');

const logger = new Logger('AudioLevelObserver');

class AudioLevelObserver extends RtpObserver
{
	/**
	 * @private
	 *
	 * @emits {volumes: Array<Object<producer: Producer, volume: Number>>} volumes
	 * @emits silence
	 */
	constructor({ ...params })
	{
		super(params);
	}

	/**
	 * @private
	 * @override
	 */
	_handleWorkerNotifications()
	{
		this._channel.on(this._internal.rtpObserverId, (event, data) =>
		{
			switch (event)
			{
				case 'volumes':
				{
					// Get the corresponding Producer instance and remove entries with
					// no Producer (it may have been closed in the meanwhile).
					const volumes = data
						.map(({ producerId, volume }) => (
							{
								producer : this._getProducerById(producerId),
								volume
							}
						))
						.filter(({ producer }) => producer);

					if (volumes.length > 0)
						this.safeEmit('volumes', volumes);

					break;
				}

				case 'silence':
				{
					this.safeEmit('silence');

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

module.exports = AudioLevelObserver;
