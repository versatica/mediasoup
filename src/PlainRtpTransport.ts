import Logger from './Logger';
import EnhancedEventEmitter from './EnhancedEventEmitter';
import Transport, {
	TransportListenIp,
	TransportTuple,
	TransportPacketEventData,
	SctpState
} from './Transport';
import Consumer, { ConsumerOptions } from './Consumer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';

export interface PlainRtpTransportOptions
{
	/**
	 * Listening IP address.
	 */
	listenIp: TransportListenIp | string;

	/**
	 * Use RTCP-mux (RTP and RTCP in the same port). Default true.
	 */
	rtcpMux?: boolean;

	/**
	 * Whether remote IP:port should be auto-detected based on first RTP/RTCP
	 * packet received. If enabled, connect() method must not be called. This
	 * option is ignored if multiSource is set. Default false.
	 */
	comedia?: boolean;

	/**
	 * Whether RTP/RTCP from different remote IPs:ports is allowed. If set, the
	 * transport will just be valid for receiving media (consume() cannot be
	 * called on it) and connect() must not be called. Default false.
	 */
	multiSource?: boolean;

	/**
	 * Create a SCTP association. Default false.
	 */
	enableSctp?: boolean;

	/**
	 * SCTP streams number.
	 */
	numSctpStreams?: NumSctpStreams;

	/**
	 * Maximum size of data that can be passed to DataProducer's send() method.
	 * Default 262144.
	 */
	maxSctpMessageSize?: number;

	/**
	 * Custom application data.
	 */
	appData?: any;
}

export interface PlainRtpTransportStat
{
	// Common to all Transports.
	type: string;
	transportId: string;
	timestamp: number;
	sctpState?: SctpState;
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

	// PlainRtpTransport specific.
	rtcpMux: boolean;
	comedia: boolean;
	multiSource: boolean;
	tuple: TransportTuple;
	rtcpTuple?: TransportTuple;
}

const logger = new Logger('PlainRtpTransport');

export default class PlainRtpTransport extends Transport
{
	// PlainRtpTransport data.
	// - .rtcpMux
	// - .comedia
	// - .multiSource
	// - .tuple
	//   - .localIp
	//   - .localPort
	//   - .remoteIp
	//   - .remotePort
	//   - .protocol
	// - .rtcpTuple
	//   - .localIp
	//   - .localPort
	//   - .remoteIp
	//   - .remotePort
	//   - .protocol
	// - .sctpParameters
	//   - .port
	//   - .OS
	//   - .MIS
	//   - .maxMessageSize
	// - .sctpState

	/**
	 * @private
	 * @emits {sctpState: SctpState} sctpstatechange
	 * @emits {TransportPacketEventData} packet
	 */
	constructor(params: any)
	{
		super(params);

		logger.debug('constructor()');

		const { data } = params;

		this._data =
		{
			tuple          : data.tuple,
			rtcpTuple      : data.rtcpTuple,
			sctpParameters : data.sctpParameters,
			sctpState      : data.sctpState
		};

		this._handleWorkerNotifications();
	}

	/**
	 * Transport tuple.
	 */
	get tuple(): TransportTuple
	{
		return this._data.tuple;
	}

	/**
	 * Transport RTCP tuple.
	 */
	get rtcpTuple(): TransportTuple
	{
		return this._data.rtcpTuple;
	}

	/**
	 * SCTP parameters.
	 */
	get sctpParameters(): SctpParameters | undefined
	{
		return this._data.sctpParameters;
	}

	/**
	 * SCTP state.
	 */
	get sctpState(): SctpState
	{
		return this._data.sctpState;
	}

	/**
	 * Observer.
	 *
	 * @override
	 * @emits close
	 * @emits {producer: Producer} newproducer
	 * @emits {consumer: Consumer} newconsumer
	 * @emits {producer: DataProducer} newdataproducer
	 * @emits {consumer: DataConsumer} newdataconsumer
	 * @emits {sctpState: SctpState} sctpstatechange
	 * @emits {TransportPacketEventData} packet
	 */
	get observer(): EnhancedEventEmitter
	{
		return this._observer;
	}

	/**
	 * Close the PlainRtpTransport.
	 *
	 * @override
	 */
	close(): void
	{
		if (this._closed)
			return;

		if (this._data.sctpState)
			this._data.sctpState = 'closed';

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

		if (this._data.sctpState)
			this._data.sctpState = 'closed';

		super.routerClosed();
	}

	/**
	 * Get PlainRtpTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<PlainRtpTransportStat[]>
	{
		logger.debug('getStats()');

		return this._channel.request('transport.getStats', this._internal);
	}

	/**
	 * Provide the PlainRtpTransport remote parameters.
	 *
	 * @override
	 */
	async connect(
		{
			ip,
			port,
			rtcpPort
		}:
		{
			ip: string;
			port: number;
			rtcpPort?: number;
		}
	): Promise<void>
	{
		logger.debug('connect()');

		const reqData = { ip, port, rtcpPort };

		const data =
			await this._channel.request('transport.connect', this._internal, reqData);

		// Update data.
		this._data.tuple = data.tuple;
		this._data.rtcpTuple = data.rtcpTuple;
	}

	/**
	 * Override Transport.consume() method to reject it if multiSource is set.
	 *
	 * @override
	 */
	async consume(params: ConsumerOptions): Promise<Consumer>
	{
		if (this._data.multiSource)
			throw new Error('cannot call consume() with multiSource set');

		return super.consume(params);
	}

	private _handleWorkerNotifications(): void
	{
		this._channel.on(this._internal.transportId, (event: string, data?: any) =>
		{
			switch (event)
			{
				case 'sctpstatechange':
				{
					const sctpState = data.sctpState as SctpState;

					this._data.sctpState = sctpState;

					this.safeEmit('sctpstatechange', sctpState);

					// Emit observer event.
					this._observer.safeEmit('sctpstatechange', sctpState);

					break;
				}

				case 'packet':
				{
					const packet = data as TransportPacketEventData;

					this.safeEmit('packet', packet);

					// Emit observer event.
					this._observer.safeEmit('packet', packet);

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
