import Logger from './Logger';
import EnhancedEventEmitter from './EnhancedEventEmitter';
//import * as ortc from './ortc';
import Transport, {
	TransportListenIp
} from './Transport';


export interface ShmTransportOptions {
  /**
   * Listening IP address.
   */
  listenIp: TransportListenIp | string;

  shmName: string;

  logName: string;

  logLevel: number;

  /**
   * Custom application data.
   */
  appData?: any;
}

const logger = new Logger('ShmTransport');

export interface ShmTransportStat
{
  // Common to all Transports.
	type: string;
	transportId: string;
	timestamp: number;
	bytesReceived: number;
	recvBitrate: number;
	bytesSent: number;
	sendBitrate: number;
	rtpBytesReceived: number;
	rtpRecvBitrate: number;
	rtpBytesSent: number;
	rtpSendBitrate: number;
	rtxBytesReceived: number;
	rtxRecvBitrate: number;
	rtxBytesSent: number;
	rtxSendBitrate: number;
	probationBytesReceived: number;
	probationRecvBitrate: number;
	probationBytesSent: number;
	probationSendBitrate: number;
	availableOutgoingBitrate?: number;
	availableIncomingBitrate?: number;
  maxIncomingBitrate?: number;
  
  //ShmTrtansport specific
  shm: string;    // shm file name
  writer: number; // writer status: initialized, closed, undefined
}

export default class ShmTransport extends Transport
{
	/**
	 * @private
	 *
	 * @emits {sctpState: String} sctpstatechange
	 */
	constructor(params: any)
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
	get observer(): EnhancedEventEmitter
	{
		return this._observer;
	}

	/**
	 * Close the ShmTransport.
	 *
	 * @override
	 */
	close(): void
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
	routerClosed(): void
	{
		if (this._closed)
			return;

		super.routerClosed();
	}

	/**
	 * Get PipeTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<ShmTransportStat[]>
	{
		logger.debug('getStats()');

		return this._channel.request('transport.getStats', this._internal);
	}

	/**
	 * Provide the ShmTransport remote parameters.
	 *
	 * @param {String} shm- shm name.
	 *
	 * @async
	 * @override
	 */
	async connect(
    {
      shm
    }:
    {
      shm: string
    })
	{
		logger.debug('connect()');

		const reqData = { shm };

		await this._channel.request('transport.connect', this._internal, reqData);
	}


	/**
	 * @private
	 * @override
	 */
	async _handleWorkerNotifications(): Promise<void>
	{
		this._channel.on(this._internal.transportId, async (event, data) => {
			switch (event)
			{
				case 'writestreammetadata':
				{
					const reqdata = data;

          await this._channel.request('transport.writestreammetadata', this._internal, reqdata); //TODO: so... let's write stream metadata via transport.writemetadata command
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
