import * as flatbuffers from 'flatbuffers';
import { Logger } from './Logger';
import { EnhancedEventEmitter } from './enhancedEvents';
import {
	parseSctpState,
	parseBaseTransportDump,
	parseBaseTransportStats,
	parseProtocol,
	parseTransportTraceEventData,
	parseTuple,
	BaseTransportDump,
	BaseTransportStats,
	Transport,
	TransportListenInfo,
	TransportListenIp,
	TransportProtocol,
	TransportTuple,
	TransportEvents,
	TransportObserverEvents,
	TransportConstructorOptions,
	SctpState,
} from './Transport';
import { WebRtcServer } from './WebRtcServer';
import { SctpParameters, NumSctpStreams } from './SctpParameters';
import { AppData, Either } from './types';
import { parseVector } from './utils';
import { Event, Notification } from './fbs/notification';
import * as FbsRequest from './fbs/request';
import * as FbsTransport from './fbs/transport';
import * as FbsWebRtcTransport from './fbs/web-rtc-transport';
import { DtlsState as FbsDtlsState } from './fbs/web-rtc-transport/dtls-state';
import { DtlsRole as FbsDtlsRole } from './fbs/web-rtc-transport/dtls-role';
import { FingerprintAlgorithm as FbsFingerprintAlgorithm } from './fbs/web-rtc-transport/fingerprint-algorithm';
import { IceState as FbsIceState } from './fbs/web-rtc-transport/ice-state';
import { IceRole as FbsIceRole } from './fbs/web-rtc-transport/ice-role';
import { IceCandidateType as FbsIceCandidateType } from './fbs/web-rtc-transport/ice-candidate-type';
import { IceCandidateTcpType as FbsIceCandidateTcpType } from './fbs/web-rtc-transport/ice-candidate-tcp-type';

export type WebRtcTransportOptions<
	WebRtcTransportAppData extends AppData = AppData,
> = WebRtcTransportOptionsBase<WebRtcTransportAppData> & WebRtcTransportListen;

type WebRtcTransportListenIndividualListenInfo = {
	/**
	 * Listening info.
	 */
	listenInfos: TransportListenInfo[];
};

type WebRtcTransportListenIndividualListenIp = {
	/**
	 * Listening IP address or addresses in order of preference (first one is the
	 * preferred one).
	 */
	listenIps: (TransportListenIp | string)[];

	/**
	 * Fixed port to listen on instead of selecting automatically from Worker's port
	 * range.
	 */
	port?: number;
};

type WebRtcTransportListenServer = {
	/**
	 * Instance of WebRtcServer.
	 */
	webRtcServer: WebRtcServer;
};

type WebRtcTransportListen = Either<
	Either<
		WebRtcTransportListenIndividualListenInfo,
		WebRtcTransportListenIndividualListenIp
	>,
	WebRtcTransportListenServer
>;

export type WebRtcTransportOptionsBase<WebRtcTransportAppData> = {
	/**
	 * Listen in UDP. Default true.
	 */
	enableUdp?: boolean;

	/**
	 * Listen in TCP. Default true if webrtcServer is given, false otherwise.
	 */
	enableTcp?: boolean;

	/**
	 * Prefer UDP. Default false.
	 */
	preferUdp?: boolean;

	/**
	 * Prefer TCP. Default false.
	 */
	preferTcp?: boolean;

	/**
	 * ICE consent timeout (in seconds). If 0 it is disabled. Default 30.
	 */
	iceConsentTimeout?: number;

	/**
	 * Initial available outgoing bitrate (in bps). Default 600000.
	 */
	initialAvailableOutgoingBitrate?: number;

	/**
	 * Create a SCTP association. Default false.
	 */
	enableSctp?: boolean;

	/**
	 * SCTP streams number.
	 */
	numSctpStreams?: NumSctpStreams;

	/**
	 * Maximum allowed size for SCTP messages sent by DataProducers.
	 * Default 262144.
	 */
	maxSctpMessageSize?: number;

	/**
	 * Maximum SCTP send buffer used by DataConsumers.
	 * Default 262144.
	 */
	sctpSendBufferSize?: number;

	/**
	 * Custom application data.
	 */
	appData?: WebRtcTransportAppData;
};

export type IceParameters = {
	usernameFragment: string;
	password: string;
	iceLite?: boolean;
};

export type IceCandidate = {
	foundation: string;
	priority: number;
	// @deprecated Use |address| instead.
	ip: string;
	address: string;
	protocol: TransportProtocol;
	port: number;
	type: IceCandidateType;
	tcpType?: IceCandidateTcpType;
};

export type DtlsParameters = {
	role?: DtlsRole;
	fingerprints: DtlsFingerprint[];
};

/**
 * The hash function algorithm (as defined in the "Hash function Textual Names"
 * registry initially specified in RFC 4572 Section 8).
 */
export type FingerprintAlgorithm =
	| 'sha-1'
	| 'sha-224'
	| 'sha-256'
	| 'sha-384'
	| 'sha-512';

/**
 * The hash function algorithm and its corresponding certificate fingerprint
 * value (in lowercase hex string as expressed utilizing the syntax of
 * "fingerprint" in RFC 4572 Section 5).
 */
export type DtlsFingerprint = {
	algorithm: FingerprintAlgorithm;
	value: string;
};

export type IceRole = 'controlled' | 'controlling';

export type IceState =
	| 'new'
	| 'connected'
	| 'completed'
	| 'disconnected'
	| 'closed';

export type IceCandidateType = 'host';

export type IceCandidateTcpType = 'passive';

export type DtlsRole = 'auto' | 'client' | 'server';

export type DtlsState =
	| 'new'
	| 'connecting'
	| 'connected'
	| 'failed'
	| 'closed';

export type WebRtcTransportStat = BaseTransportStats & {
	type: string;
	iceRole: string;
	iceState: IceState;
	iceSelectedTuple?: TransportTuple;
	dtlsState: DtlsState;
};

export type WebRtcTransportEvents = TransportEvents & {
	icestatechange: [IceState];
	iceselectedtuplechange: [TransportTuple];
	dtlsstatechange: [DtlsState];
	sctpstatechange: [SctpState];
};

export type WebRtcTransportObserver =
	EnhancedEventEmitter<WebRtcTransportObserverEvents>;

export type WebRtcTransportObserverEvents = TransportObserverEvents & {
	icestatechange: [IceState];
	iceselectedtuplechange: [TransportTuple];
	dtlsstatechange: [DtlsState];
	sctpstatechange: [SctpState];
};

type WebRtcTransportConstructorOptions<WebRtcTransportAppData> =
	TransportConstructorOptions<WebRtcTransportAppData> & {
		data: WebRtcTransportData;
	};

export type WebRtcTransportData = {
	iceRole: 'controlled';
	iceParameters: IceParameters;
	iceCandidates: IceCandidate[];
	iceState: IceState;
	iceSelectedTuple?: TransportTuple;
	dtlsParameters: DtlsParameters;
	dtlsState: DtlsState;
	dtlsRemoteCert?: string;
	sctpParameters?: SctpParameters;
	sctpState?: SctpState;
};

type WebRtcTransportDump = BaseTransportDump & {
	iceRole: 'controlled';
	iceParameters: IceParameters;
	iceCandidates: IceCandidate[];
	iceState: IceState;
	iceSelectedTuple?: TransportTuple;
	dtlsParameters: DtlsParameters;
	dtlsState: DtlsState;
	dtlsRemoteCert?: string;
};

const logger = new Logger('WebRtcTransport');

export class WebRtcTransport<
	WebRtcTransportAppData extends AppData = AppData,
> extends Transport<
	WebRtcTransportAppData,
	WebRtcTransportEvents,
	WebRtcTransportObserver
> {
	// WebRtcTransport data.
	readonly #data: WebRtcTransportData;

	/**
	 * @private
	 */
	constructor(
		options: WebRtcTransportConstructorOptions<WebRtcTransportAppData>
	) {
		const observer: WebRtcTransportObserver =
			new EnhancedEventEmitter<WebRtcTransportObserverEvents>();

		super(options, observer);

		logger.debug('constructor()');

		const { data } = options;

		this.#data = {
			iceRole: data.iceRole,
			iceParameters: data.iceParameters,
			iceCandidates: data.iceCandidates,
			iceState: data.iceState,
			iceSelectedTuple: data.iceSelectedTuple,
			dtlsParameters: data.dtlsParameters,
			dtlsState: data.dtlsState,
			dtlsRemoteCert: data.dtlsRemoteCert,
			sctpParameters: data.sctpParameters,
			sctpState: data.sctpState,
		};

		this.handleWorkerNotifications();
	}

	/**
	 * Observer.
	 *
	 * @override
	 */
	get observer(): WebRtcTransportObserver {
		return super.observer;
	}

	/**
	 * ICE role.
	 */
	get iceRole(): 'controlled' {
		return this.#data.iceRole;
	}

	/**
	 * ICE parameters.
	 */
	get iceParameters(): IceParameters {
		return this.#data.iceParameters;
	}

	/**
	 * ICE candidates.
	 */
	get iceCandidates(): IceCandidate[] {
		return this.#data.iceCandidates;
	}

	/**
	 * ICE state.
	 */
	get iceState(): IceState {
		return this.#data.iceState;
	}

	/**
	 * ICE selected tuple.
	 */
	get iceSelectedTuple(): TransportTuple | undefined {
		return this.#data.iceSelectedTuple;
	}

	/**
	 * DTLS parameters.
	 */
	get dtlsParameters(): DtlsParameters {
		return this.#data.dtlsParameters;
	}

	/**
	 * DTLS state.
	 */
	get dtlsState(): DtlsState {
		return this.#data.dtlsState;
	}

	/**
	 * Remote certificate in PEM format.
	 */
	get dtlsRemoteCert(): string | undefined {
		return this.#data.dtlsRemoteCert;
	}

	/**
	 * SCTP parameters.
	 */
	get sctpParameters(): SctpParameters | undefined {
		return this.#data.sctpParameters;
	}

	/**
	 * SCTP state.
	 */
	get sctpState(): SctpState | undefined {
		return this.#data.sctpState;
	}

	/**
	 * Close the WebRtcTransport.
	 *
	 * @override
	 */
	close(): void {
		if (this.closed) {
			return;
		}

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState) {
			this.#data.sctpState = 'closed';
		}

		super.close();
	}

	/**
	 * Router was closed.
	 *
	 * @private
	 * @override
	 */
	routerClosed(): void {
		if (this.closed) {
			return;
		}

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState) {
			this.#data.sctpState = 'closed';
		}

		super.routerClosed();
	}

	/**
	 * Called when closing the associated listenServer (WebRtcServer).
	 *
	 * @private
	 */
	listenServerClosed(): void {
		if (this.closed) {
			return;
		}

		this.#data.iceState = 'closed';
		this.#data.iceSelectedTuple = undefined;
		this.#data.dtlsState = 'closed';

		if (this.#data.sctpState) {
			this.#data.sctpState = 'closed';
		}

		super.listenServerClosed();
	}

	/**
	 * Dump Transport.
	 */
	async dump(): Promise<WebRtcTransportDump> {
		logger.debug('dump()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_DUMP,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsWebRtcTransport.DumpResponse();

		response.body(data);

		return parseWebRtcTransportDumpResponse(data);
	}

	/**
	 * Get WebRtcTransport stats.
	 *
	 * @override
	 */
	async getStats(): Promise<WebRtcTransportStat[]> {
		logger.debug('getStats()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_GET_STATS,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsWebRtcTransport.GetStatsResponse();

		response.body(data);

		return [parseGetStatsResponse(data)];
	}

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @override
	 */
	async connect({
		dtlsParameters,
	}: {
		dtlsParameters: DtlsParameters;
	}): Promise<void> {
		logger.debug('connect()');

		const requestOffset = createConnectRequest({
			builder: this.channel.bufferBuilder,
			dtlsParameters,
		});

		// Wait for response.
		const response = await this.channel.request(
			FbsRequest.Method.WEBRTCTRANSPORT_CONNECT,
			FbsRequest.Body.WebRtcTransport_ConnectRequest,
			requestOffset,
			this.internal.transportId
		);

		/* Decode Response. */
		const data = new FbsWebRtcTransport.ConnectResponse();

		response.body(data);

		// Update data.
		this.#data.dtlsParameters.role = dtlsRoleFromFbs(data.dtlsLocalRole());
	}

	/**
	 * Restart ICE.
	 */
	async restartIce(): Promise<IceParameters> {
		logger.debug('restartIce()');

		const response = await this.channel.request(
			FbsRequest.Method.TRANSPORT_RESTART_ICE,
			undefined,
			undefined,
			this.internal.transportId
		);

		/* Decode Response. */
		const restartIceResponse = new FbsTransport.RestartIceResponse();

		response.body(restartIceResponse);

		const iceParameters = {
			usernameFragment: restartIceResponse.usernameFragment()!,
			password: restartIceResponse.password()!,
			iceLite: restartIceResponse.iceLite(),
		};

		this.#data.iceParameters = iceParameters;

		return iceParameters;
	}

	private handleWorkerNotifications(): void {
		this.channel.on(
			this.internal.transportId,
			(event: Event, data?: Notification) => {
				switch (event) {
					case Event.WEBRTCTRANSPORT_ICE_STATE_CHANGE: {
						const notification =
							new FbsWebRtcTransport.IceStateChangeNotification();

						data!.body(notification);

						const iceState = iceStateFromFbs(notification.iceState());

						this.#data.iceState = iceState;

						this.safeEmit('icestatechange', iceState);

						// Emit observer event.
						this.observer.safeEmit('icestatechange', iceState);

						break;
					}

					case Event.WEBRTCTRANSPORT_ICE_SELECTED_TUPLE_CHANGE: {
						const notification =
							new FbsWebRtcTransport.IceSelectedTupleChangeNotification();

						data!.body(notification);

						const iceSelectedTuple = parseTuple(notification.tuple()!);

						this.#data.iceSelectedTuple = iceSelectedTuple;

						this.safeEmit('iceselectedtuplechange', iceSelectedTuple);

						// Emit observer event.
						this.observer.safeEmit('iceselectedtuplechange', iceSelectedTuple);

						break;
					}

					case Event.WEBRTCTRANSPORT_DTLS_STATE_CHANGE: {
						const notification =
							new FbsWebRtcTransport.DtlsStateChangeNotification();

						data!.body(notification);

						const dtlsState = dtlsStateFromFbs(notification.dtlsState());

						this.#data.dtlsState = dtlsState;

						if (dtlsState === 'connected') {
							this.#data.dtlsRemoteCert = notification.remoteCert()!;
						}

						this.safeEmit('dtlsstatechange', dtlsState);

						// Emit observer event.
						this.observer.safeEmit('dtlsstatechange', dtlsState);

						break;
					}

					case Event.TRANSPORT_SCTP_STATE_CHANGE: {
						const notification = new FbsTransport.SctpStateChangeNotification();

						data!.body(notification);

						const sctpState = parseSctpState(notification.sctpState());

						this.#data.sctpState = sctpState;

						this.safeEmit('sctpstatechange', sctpState);

						// Emit observer event.
						this.observer.safeEmit('sctpstatechange', sctpState);

						break;
					}

					case Event.TRANSPORT_TRACE: {
						const notification = new FbsTransport.TraceNotification();

						data!.body(notification);

						const trace = parseTransportTraceEventData(notification);

						this.safeEmit('trace', trace);

						// Emit observer event.
						this.observer.safeEmit('trace', trace);

						break;
					}

					default: {
						logger.error('ignoring unknown event "%s"', event);
					}
				}
			}
		);
	}
}

function iceStateFromFbs(fbsIceState: FbsIceState): IceState {
	switch (fbsIceState) {
		case FbsIceState.NEW: {
			return 'new';
		}

		case FbsIceState.CONNECTED: {
			return 'connected';
		}

		case FbsIceState.COMPLETED: {
			return 'completed';
		}

		case FbsIceState.DISCONNECTED: {
			return 'disconnected';
		}
	}
}

function iceRoleFromFbs(role: FbsIceRole): IceRole {
	switch (role) {
		case FbsIceRole.CONTROLLED: {
			return 'controlled';
		}

		case FbsIceRole.CONTROLLING: {
			return 'controlling';
		}
	}
}

function iceCandidateTypeFromFbs(type: FbsIceCandidateType): IceCandidateType {
	switch (type) {
		case FbsIceCandidateType.HOST: {
			return 'host';
		}
	}
}

function iceCandidateTcpTypeFromFbs(
	type: FbsIceCandidateTcpType
): IceCandidateTcpType {
	switch (type) {
		case FbsIceCandidateTcpType.PASSIVE: {
			return 'passive';
		}
	}
}

function dtlsStateFromFbs(fbsDtlsState: FbsDtlsState): DtlsState {
	switch (fbsDtlsState) {
		case FbsDtlsState.NEW: {
			return 'new';
		}

		case FbsDtlsState.CONNECTING: {
			return 'connecting';
		}

		case FbsDtlsState.CONNECTED: {
			return 'connected';
		}

		case FbsDtlsState.FAILED: {
			return 'failed';
		}

		case FbsDtlsState.CLOSED: {
			return 'closed';
		}
	}
}

function dtlsRoleFromFbs(role: FbsDtlsRole): DtlsRole {
	switch (role) {
		case FbsDtlsRole.AUTO: {
			return 'auto';
		}

		case FbsDtlsRole.CLIENT: {
			return 'client';
		}

		case FbsDtlsRole.SERVER: {
			return 'server';
		}
	}
}

function fingerprintAlgorithmsFromFbs(
	algorithm: FbsFingerprintAlgorithm
): FingerprintAlgorithm {
	switch (algorithm) {
		case FbsFingerprintAlgorithm.SHA1: {
			return 'sha-1';
		}

		case FbsFingerprintAlgorithm.SHA224: {
			return 'sha-224';
		}

		case FbsFingerprintAlgorithm.SHA256: {
			return 'sha-256';
		}

		case FbsFingerprintAlgorithm.SHA384: {
			return 'sha-384';
		}

		case FbsFingerprintAlgorithm.SHA512: {
			return 'sha-512';
		}
	}
}

function fingerprintAlgorithmToFbs(
	algorithm: FingerprintAlgorithm
): FbsFingerprintAlgorithm {
	switch (algorithm) {
		case 'sha-1': {
			return FbsFingerprintAlgorithm.SHA1;
		}

		case 'sha-224': {
			return FbsFingerprintAlgorithm.SHA224;
		}

		case 'sha-256': {
			return FbsFingerprintAlgorithm.SHA256;
		}

		case 'sha-384': {
			return FbsFingerprintAlgorithm.SHA384;
		}

		case 'sha-512': {
			return FbsFingerprintAlgorithm.SHA512;
		}

		default: {
			throw new TypeError(`invalid FingerprintAlgorithm: ${algorithm}`);
		}
	}
}

function dtlsRoleToFbs(role: DtlsRole): FbsDtlsRole {
	switch (role) {
		case 'auto': {
			return FbsDtlsRole.AUTO;
		}

		case 'client': {
			return FbsDtlsRole.CLIENT;
		}

		case 'server': {
			return FbsDtlsRole.SERVER;
		}

		default: {
			throw new TypeError(`invalid DtlsRole: ${role}`);
		}
	}
}

export function parseWebRtcTransportDumpResponse(
	binary: FbsWebRtcTransport.DumpResponse
): WebRtcTransportDump {
	// Retrieve BaseTransportDump.
	const baseTransportDump = parseBaseTransportDump(binary.base()!);
	// Retrieve ICE candidates.
	const iceCandidates = parseVector<IceCandidate>(
		binary,
		'iceCandidates',
		parseIceCandidate
	);
	// Retrieve ICE parameters.
	const iceParameters = parseIceParameters(binary.iceParameters()!);
	// Retrieve DTLS parameters.
	const dtlsParameters = parseDtlsParameters(binary.dtlsParameters()!);

	return {
		...baseTransportDump,
		sctpParameters: baseTransportDump.sctpParameters,
		sctpState: baseTransportDump.sctpState,
		iceRole: 'controlled',
		iceParameters: iceParameters,
		iceCandidates: iceCandidates,
		iceState: iceStateFromFbs(binary.iceState()),
		dtlsParameters: dtlsParameters,
		dtlsState: dtlsStateFromFbs(binary.dtlsState()),
	};
}

function createConnectRequest({
	builder,
	dtlsParameters,
}: {
	builder: flatbuffers.Builder;
	dtlsParameters: DtlsParameters;
}): number {
	// Serialize DtlsParameters. This can throw.
	const dtlsParametersOffset = serializeDtlsParameters(builder, dtlsParameters);

	return FbsWebRtcTransport.ConnectRequest.createConnectRequest(
		builder,
		dtlsParametersOffset
	);
}

function parseGetStatsResponse(
	binary: FbsWebRtcTransport.GetStatsResponse
): WebRtcTransportStat {
	const base = parseBaseTransportStats(binary.base()!);

	return {
		...base,
		type: 'webrtc-transport',
		iceRole: iceRoleFromFbs(binary.iceRole()),
		iceState: iceStateFromFbs(binary.iceState()),
		iceSelectedTuple: binary.iceSelectedTuple()
			? parseTuple(binary.iceSelectedTuple()!)
			: undefined,
		dtlsState: dtlsStateFromFbs(binary.dtlsState()),
	};
}

function parseIceCandidate(
	binary: FbsWebRtcTransport.IceCandidate
): IceCandidate {
	return {
		foundation: binary.foundation()!,
		priority: binary.priority(),
		ip: binary.address()!,
		address: binary.address()!,
		protocol: parseProtocol(binary.protocol()),
		port: binary.port(),
		type: iceCandidateTypeFromFbs(binary.type()),
		tcpType:
			binary.tcpType() === null
				? undefined
				: iceCandidateTcpTypeFromFbs(binary.tcpType()!),
	};
}

function parseIceParameters(
	binary: FbsWebRtcTransport.IceParameters
): IceParameters {
	return {
		usernameFragment: binary.usernameFragment()!,
		password: binary.password()!,
		iceLite: binary.iceLite(),
	};
}

function parseDtlsParameters(
	binary: FbsWebRtcTransport.DtlsParameters
): DtlsParameters {
	const fingerprints: DtlsFingerprint[] = [];

	for (let i = 0; i < binary.fingerprintsLength(); ++i) {
		const fbsFingerprint = binary.fingerprints(i)!;
		const fingerPrint: DtlsFingerprint = {
			algorithm: fingerprintAlgorithmsFromFbs(fbsFingerprint.algorithm()),
			value: fbsFingerprint.value()!,
		};

		fingerprints.push(fingerPrint);
	}

	return {
		fingerprints: fingerprints,
		role: binary.role() === null ? undefined : dtlsRoleFromFbs(binary.role()!),
	};
}

function serializeDtlsParameters(
	builder: flatbuffers.Builder,
	dtlsParameters: DtlsParameters
): number {
	const fingerprints: number[] = [];

	for (const fingerprint of dtlsParameters.fingerprints) {
		const algorithm = fingerprintAlgorithmToFbs(fingerprint.algorithm);
		const valueOffset = builder.createString(fingerprint.value);
		const fingerprintOffset = FbsWebRtcTransport.Fingerprint.createFingerprint(
			builder,
			algorithm,
			valueOffset
		);

		fingerprints.push(fingerprintOffset);
	}

	const fingerprintsOffset =
		FbsWebRtcTransport.DtlsParameters.createFingerprintsVector(
			builder,
			fingerprints
		);

	const role =
		dtlsParameters.role !== undefined
			? dtlsRoleToFbs(dtlsParameters.role)
			: FbsWebRtcTransport.DtlsRole.AUTO;

	return FbsWebRtcTransport.DtlsParameters.createDtlsParameters(
		builder,
		fingerprintsOffset,
		role
	);
}
