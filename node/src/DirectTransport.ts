import { Logger } from './Logger';
import { UnsupportedError } from './errors';
import {
	BaseTransportDump,
	parseBaseTransportDump,
	Transport,
	TransportTraceEventData,
	TransportEvents,
	TransportObserverEvents,
	TransportConstructorOptions
} from './Transport';
import { SctpParameters } from './SctpParameters';
import * as FbsTransport from './fbs/transport_generated';
import * as FbsRequest from './fbs/request_generated';

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
	appData?: Record<string, unknown>;
};

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
};

export type DirectTransportEvents = TransportEvents &
{
	rtcp: [Buffer];
};

export type DirectTransportObserverEvents = TransportObserverEvents &
{
	rtcp: [Buffer];
};

type DirectTransportConstructorOptions = TransportConstructorOptions &
{
	data: DirectTransportData;
};

export type DirectTransportData =
{
	sctpParameters?: SctpParameters;
};

const logger = new Logger('DirectTransport');

export class DirectTransport extends
	Transport<DirectTransportEvents, DirectTransportObserverEvents>
{
	// DirectTransport data.
	readonly #data: DirectTransportData;

	/**
	 * @private
	 */
	constructor(options: DirectTransportConstructorOptions)
	{
		super(options);

		logger.debug('constructor()');

		this.#data =
		{
			// Nothing.
		};

		this.handleWorkerNotifications();
	}

	/**
	 * Close the DirectTransport.
	 *
	 * @override
	 */
	close(): void
	{
		if (this.closed)
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
		if (this.closed)
			return;

		super.routerClosed();
	}

	/**
	 * Dump Transport.
	 */
	async dump(): Promise<any>
	{
		logger.debug('dump()');

		const response = await this.channel.requestBinary(
			FbsRequest.Method.TRANSPORT_DUMP,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode the response. */
		const dump = new FbsTransport.DumpResponse();

		response.body(dump);

		const transportDump = new FbsTransport.DirectTransportDump();

		dump.data(transportDump);

		return parseDirectTransportDump(transportDump);
	}

	/**
	 * Get DirectTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<DirectTransportStat[]>
	{
		logger.debug('getStats()');

		const response = await this.channel.requestBinary(
			FbsRequest.Method.TRANSPORT_GET_STATS,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode the response. */
		const data = new FbsTransport.GetStatsResponse();

		response.body(data);

		return JSON.parse(data.stats()!);
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
	 * @override
	 */
	// eslint-disable-next-line @typescript-eslint/no-unused-vars
	async setMaxOutgoingBitrate(bitrate: number): Promise<void>
	{
		throw new UnsupportedError(
			'setMaxOutgoingBitrate() not implemented in DirectTransport');
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

		this.payloadChannel.notify(
			'transport.sendRtcp', this.internal.transportId, undefined, rtcpPacket);
	}

	private handleWorkerNotifications(): void
	{
		this.channel.on(this.internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case 'trace':
				{
					const trace = data as TransportTraceEventData;

					this.safeEmit('trace', trace);

					// Emit observer event.
					this.observer.safeEmit('trace', trace);

					break;
				}

				default:
				{
					logger.error('ignoring unknown event "%s"', event);
				}
			}
		});

		this.payloadChannel.on(
			this.internal.transportId,
			(event: string, data: any | undefined, payload: Buffer) =>
			{
				switch (event)
				{
					case 'rtcp':
					{
						if (this.closed)
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

export function parseDirectTransportDump(
	binary: FbsTransport.DirectTransportDump
): BaseTransportDump
{
	// Retrieve BaseTransportDump.
	const fbsBaseTransportDump = new FbsTransport.BaseTransportDump();

	binary.base()!.data(fbsBaseTransportDump);

	return parseBaseTransportDump(fbsBaseTransportDump);
}
