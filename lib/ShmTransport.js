const Logger = require('./Logger');
const Transport = require('./Transport');

const logger = new Logger('ShmTransport');

class ShmTransport extends Transport
{
	/**
	 * @private
	 *
	 * @emits {sctpState: String} sctpstatechange
	 */
	constructor(params)
	{
		super(params);

		logger.debug('constructor()');

		const { data } = params;

		// ShmTransport data.
		// See sfushm_av_media.h for details
		// TODO: some may not be needed here, such as ssrc?
		// @type {Object}
		// - .shm
		//   - .name
		// - .log
		//   - .fileName (can be 'stdout' to redirect log output)
		//   - .level
		// - .channels[]
		//  - .target_buf_ms
		//  - .target_kpbs
		//  - .ssrc
		//  - .sample_rate
		//  - .num_audio_chn
		//  - .media_type (audio or video; TODO: smth to be done with rtcp and metadata?)
		// TODO: number of channels?

		this._data =
		{
			shm      : data.shm,
			log      : data.log,
			channels : data.channels
    };
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @type {EventEmitter}
	 *
	 * @emits close
	 * @emits {producer: Producer} newproducer
	 * @emits {consumer: Consumer} newconsumer
	 * @emits {producer: DataProducer} newdataproducer
	 * @emits {consumer: DataConsumer} newdataconsumer
	 * @emits {sctpState: String} sctpstatechange
	 */
	get observer()
	{
		return this._observer;
	}

	/**
	 * Close the ShmTransport.
	 *
	 * @override
	 */
	close()
	{
		if (this._closed)
			return;

			super.close();
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @override
	 */
	routerClosed()
	{
		if (this._closed)
			return;

		super.routerClosed();
	}

	/**
	 * Provide the ShmTransport remote parameters.
	 *
	 * @param {String} shm- shm name.
	 *
	 * @async
	 * @override
	 */
	async connect({ shm } = {})
	{
		logger.debug('connect()');

		const reqData = { shm };

		const data =
			await this._channel.request('transport.connect', this._internal, reqData);
	}

	/**
	 *
	 * @async
	 * @override
	 * @returns {Consumer}
	 */
	async consume({ ...params } = {})
	{
		params.appData = {
			type = 'shm'
		};
		return super.consume({ ...params, appData: {type: 'shm'} });
	}

	/**
	 * @private
	 * @override
	 */
	_handleWorkerNotifications()
	{
		this._channel.on(this._internal.transportId, (event, data) =>
		{
			switch (event)
			{
				case 'writestreammetadata':
				{
					const reqdata = data;

          const data =
          await this._channel.request('transport.writestreammetadata', this._internal, reqData); //TODO: so... let's write stream metadata via transport.writemetadata command
          
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

module.exports = ShmTransport;
