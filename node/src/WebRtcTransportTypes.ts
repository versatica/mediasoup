import type { EnhancedEventEmitter } from './enhancedEvents';
import type {
	Transport,
	TransportListenInfo,
	TransportListenIp,
	TransportProtocol,
	TransportTuple,
	SctpState,
	BaseTransportDump,
	BaseTransportStats,
	TransportEvents,
	TransportObserverEvents,
} from './TransportTypes';
import type { WebRtcServer } from './WebRtcServerTypes';
import type {
	SctpParameters,
	SctpNegotiatedCapabilities,
} from './sctpParametersTypes';
import type { Either, AppData } from './types';

export type WebRtcTransportOptions<
	WebRtcTransportAppData extends AppData = AppData,
> = WebRtcTransportOptionsBase<WebRtcTransportAppData> & WebRtcTransportListen;

type WebRtcTransportOptionsBase<WebRtcTransportAppData> = {
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
	 * Maximum allowed size for SCTP messages sent by DataConsumers (in bytes).
	 * Default 262144.
	 */
	maxSendMessageSize?: number;

	/**
	 * Maximum allowed size for SCTP messages received by DataProducers (in bytes).
	 * Default 262144.
	 */
	maxReceiveMessageSize?: number;

	/**
	 * Maximum SCTP send buffer used by DataConsumers (in bytes).
	 * Default 2000000.
	 */
	sctpSendBufferSize?: number;

	/**
	 * Per stream send queue size limit. Similar to `sctpSendBufferSize`, but
	 * limiting the size of individual streams.
	 * Default 2000000.
	 */
	sctpPerStreamSendQueueLimit?: number;

	/**
	 * Maximum received window buffer size (in bytes). This should be a bit larger
	 * than the largest sized message you want to be able to receive.
	 * Default 5242880.
	 */
	sctpMaxReceiverWindowBufferSize?: number;

	/**
	 * Custom application data.
	 */
	appData?: WebRtcTransportAppData;
};

type WebRtcTransportListen = Either<
	Either<
		WebRtcTransportListenIndividualListenInfo,
		WebRtcTransportListenIndividualListenIp
	>,
	WebRtcTransportListenServer
>;

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

export type WebRtcTransportDump = BaseTransportDump & {
	iceRole: 'controlled';
	iceParameters: IceParameters;
	iceCandidates: IceCandidate[];
	iceState: IceState;
	iceSelectedTuple?: TransportTuple;
	dtlsParameters: DtlsParameters;
	dtlsState: DtlsState;
	dtlsRemoteCert?: string;
};

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

export interface WebRtcTransport<
	WebRtcTransportAppData extends AppData = AppData,
> extends Transport<
	WebRtcTransportAppData,
	WebRtcTransportEvents,
	WebRtcTransportObserver
> {
	/**
	 * Transport type.
	 *
	 * @override
	 */
	get type(): 'webrtc';

	/**
	 * Observer.
	 *
	 * @override
	 */
	get observer(): WebRtcTransportObserver;

	/**
	 * ICE role.
	 */
	get iceRole(): 'controlled';

	/**
	 * ICE parameters.
	 */
	get iceParameters(): IceParameters;

	/**
	 * ICE candidates.
	 */
	get iceCandidates(): IceCandidate[];

	/**
	 * ICE state.
	 */
	get iceState(): IceState;

	/**
	 * ICE selected tuple.
	 */
	get iceSelectedTuple(): TransportTuple | undefined;

	/**
	 * DTLS parameters.
	 */
	get dtlsParameters(): DtlsParameters;

	/**
	 * DTLS state.
	 */
	get dtlsState(): DtlsState;

	/**
	 * Remote certificate in PEM format.
	 */
	get dtlsRemoteCert(): string | undefined;

	/**
	 * SCTP parameters.
	 */
	get sctpParameters(): SctpParameters | undefined;

	/**
	 * SCTP state.
	 */
	get sctpState(): SctpState | undefined;

	/**
	 * SCTP negotiated capabilities.
	 *
	 * @remarks
	 * - Only present if SCTP association is connected.
	 */
	get sctpNegotiatedCapabilities(): SctpNegotiatedCapabilities | undefined;

	/**
	 * Dump WebRtcTransport.
	 *
	 * @override
	 */
	dump(): Promise<WebRtcTransportDump>;

	/**
	 * Get WebRtcTransport stats.
	 *
	 * @override
	 */
	getStats(): Promise<WebRtcTransportStat[]>;

	/**
	 * Provide the WebRtcTransport remote parameters.
	 *
	 * @override
	 */
	connect({
		dtlsParameters,
	}: {
		dtlsParameters: DtlsParameters;
	}): Promise<void>;

	/**
	 * Restart ICE.
	 */
	restartIce(): Promise<IceParameters>;
}
