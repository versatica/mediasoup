import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
//import * as ortc from './ortc';
import { Transport,
	TransportListenIp
} from './Transport';


export interface ShmTransportOptions {
  /**
   * Listening IP address.
   */
  listenIp: TransportListenIp | string;

  shm: any;

  log: any;

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

export class ShmTransport extends Transport
{

	private _shm?: string;

	private _log?: string;

	/**
	* @private
	 *
	 */
	constructor(params: any)
	{
		super(params);

		logger.debug('constructor()');

		const { data } = params;

		// ShmTransport data.
		this._shm = data.shm.name;

		this._log = data.shm.log;
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
	 * Get ShmTransport stats.
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
	 * Provide the ShmTransport remote parameters.
	 *
	 * @param {Object} meta - metadata string.
	 *
	 * @async
	 */
	async writeStreamMetaData(
		{
      meta
    }:
    {
      meta: string;
    })
	{
		logger.debug('writeStreamMetaData()');

		const reqData = {
			meta,
			shm: this._shm,
			log: this._log
		};

		await this._channel.request('transport.consumeStreamMeta', this._internal, reqData);
	}

	/**
	 * Does nothing, should not be called like this
	 * 
	 * @private
	 * @override
	 */
	_handleWorkerNotifications(): void
	{
		this._channel.on(this._internal.transportId, async (event, data) => {
			switch (event)
			{
				default:
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});
	}
}
