import { Logger } from './Logger';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { UnsupportedError } from './errors';
import { Transport, TransportTraceEventData } from './Transport';

export type DirectTransportOptions =
{
	/**
	 * Maximum allowed size for direct messages sent from DataProducers.
	 * Default 262144.
	 */
	maxMessageSize: number;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

export type DirectTransportStat =
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
	probationBytesSent: number;
	probationSendBitrate: number;
	availableOutgoingBitrate?: number;
	availableIncomingBitrate?: number;
	maxIncomingBitrate?: number;
}

const logger = new Logger('DirectTransport');

export class DirectTransport extends Transport
{
	// DirectTransport data.
	protected readonly _data:
	{
		// Nothing for now.
	};

	/**
	 * @private
	 * @emits rtcp - (packet: Buffer)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	constructor(params: any)
	{
		super(params);

		logger.debug('constructor()');

		this._data =
		{
			// Nothing for now.
		};

		this._handleWorkerNotifications();
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @emits close
	 * @emits newdataproducer - (dataProducer: DataProducer)
	 * @emits newdataconsumer - (dataProducer: DataProducer)
	 * @emits trace - (trace: TransportTraceEventData)
	 */
	get observer(): EnhancedEventEmitter
	{
		return this._observer;
	}

	/**
	 * Close the DirectTransport.
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
	 * Get DirectTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<DirectTransportStat[]>
	{
		logger.debug('getStats()');

		return this._channel.request('transport.getStats', this._internal);
	}

	/**
	 * NO-OP method in DirectTransport.
	 *
	 * @override
	 */
	async connect(): Promise<void>
	{
		logger.debug('connect()');
	}

	/**
	 * @override
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	async setMaxIncomingBitrate(bitrate: number): Promise<void>
	{
		throw new UnsupportedError(
			'setMaxIncomingBitrate() not implemented in DirectTransport');
	}

	/**
	 * Send RTCP packet.
	 */
	sendRtcp(rtcpPacket: Buffer)
	{
		if (!Buffer.isBuffer(rtcpPacket))
		{
			throw new TypeError('rtcpPacket must be a Buffer');
		}

		this._payloadChannel.notify(
			'transport.sendRtcp', this._internal, undefined, rtcpPacket);
	}

	private _handleWorkerNotifications(): void
	{
		this._channel.on(this._internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case 'trace':
				{
					const trace = data as TransportTraceEventData;

					this.safeEmit('trace', trace);

					// Emit observer event.
					this._observer.safeEmit('trace', trace);

					break;
				}

				default:
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});

		this._payloadChannel.on(
			this._internal.transportId,
			(event: string, data: any | undefined, payload: Buffer) =>
			{
				switch (event)
				{
					case 'rtcp':
					{
						if (this._closed)
							break;

						const packet = payload;

						this.safeEmit('rtcp', packet);

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
