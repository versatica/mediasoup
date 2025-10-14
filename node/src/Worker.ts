import * as process from 'node:process';
import * as path from 'node:path';
import { WorkerChannel } from './workerChannel/src';
import { version } from './';
import { Logger } from './Logger';
import { EnhancedEventEmitter } from './enhancedEvents';
import * as ortc from './ortc';
import type {
	Worker,
	WorkerSettings,
	WorkerUpdateableSettings,
	WorkerResourceUsage,
	WorkerDump,
	WorkerEvents,
	WorkerObserver,
	WorkerObserverEvents,
} from './WorkerTypes';
import { Channel } from './Channel';
import type { WebRtcServer, WebRtcServerOptions } from './WebRtcServerTypes';
import { WebRtcServerImpl } from './WebRtcServer';
import type { Router, RouterOptions } from './RouterTypes';
import { RouterImpl } from './Router';
import { portRangeToFbs, socketFlagsToFbs } from './Transport';
import type { RouterRtpCodecCapability } from './rtpParametersTypes';
import * as utils from './utils';
import * as fbsUtils from './fbsUtils';
import type { AppData } from './types';
import { Event } from './fbs/notification';
import * as FbsRequest from './fbs/request';
import * as FbsWorker from './fbs/worker';
import * as FbsTransport from './fbs/transport';
import { Protocol as FbsTransportProtocol } from './fbs/transport/protocol';

const logger = new Logger('Worker');

export const workerBin: string = getWorkerBin();

export class WorkerImpl<WorkerAppData extends AppData = AppData>
	extends EnhancedEventEmitter<WorkerEvents>
	implements Worker
{
	// Worker process PID.
	readonly #pid: number = process.pid;

	// WorkerChannel instance.
	readonly #workerChannel: WorkerChannel;

	// Channel instance.
	readonly #channel: Channel;

	// Closed flag.
	#closed = false;

	// Custom app data.
	#appData: WorkerAppData;

	// WebRtcServers set.
	readonly #webRtcServers: Set<WebRtcServer> = new Set();

	// Routers set.
	readonly #routers: Set<Router> = new Set();

	// Observer instance.
	readonly #observer: WorkerObserver =
		new EnhancedEventEmitter<WorkerObserverEvents>();

	constructor({
		logLevel,
		logTags,
		rtcMinPort,
		rtcMaxPort,
		dtlsCertificateFile,
		dtlsPrivateKeyFile,
		libwebrtcFieldTrials,
		disableLiburing,
		appData,
	}: WorkerSettings<WorkerAppData>) {
		super();

		logger.debug('constructor()');

		let spawnBin = workerBin;
		let spawnArgs: string[] = [];

		if (process.env['MEDIASOUP_USE_VALGRIND'] === 'true') {
			spawnBin = process.env['MEDIASOUP_VALGRIND_BIN'] ?? 'valgrind';

			if (process.env['MEDIASOUP_VALGRIND_OPTIONS']) {
				spawnArgs = spawnArgs.concat(
					process.env['MEDIASOUP_VALGRIND_OPTIONS'].split(/\s+/)
				);
			}

			spawnArgs.push(workerBin);
		}

		if (typeof logLevel === 'string' && logLevel) {
			spawnArgs.push(`--logLevel=${logLevel}`);
		}

		for (const logTag of Array.isArray(logTags) ? logTags : []) {
			if (typeof logTag === 'string' && logTag) {
				spawnArgs.push(`--logTag=${logTag}`);
			}
		}

		if (typeof rtcMinPort === 'number' && !Number.isNaN(rtcMinPort)) {
			spawnArgs.push(`--rtcMinPort=${rtcMinPort}`);
		}

		if (typeof rtcMaxPort === 'number' && !Number.isNaN(rtcMaxPort)) {
			spawnArgs.push(`--rtcMaxPort=${rtcMaxPort}`);
		}

		if (typeof dtlsCertificateFile === 'string' && dtlsCertificateFile) {
			spawnArgs.push(`--dtlsCertificateFile=${dtlsCertificateFile}`);
		}

		if (typeof dtlsPrivateKeyFile === 'string' && dtlsPrivateKeyFile) {
			spawnArgs.push(`--dtlsPrivateKeyFile=${dtlsPrivateKeyFile}`);
		}

		if (typeof libwebrtcFieldTrials === 'string' && libwebrtcFieldTrials) {
			spawnArgs.push(`--libwebrtcFieldTrials=${libwebrtcFieldTrials}`);
		}

		if (disableLiburing) {
			spawnArgs.push(`--disableLiburing=true`);
		}

		logger.debug(`spawning worker process: ${spawnBin} ${spawnArgs.join(' ')}`);

		this.#workerChannel = new WorkerChannel(version, spawnArgs);

		this.#workerChannel.on('error', (code: number) => {
			if (code === 42) {
				logger.error('worker failed due to wrong settings [pid:${this.#pid}]');

				this.emit('@failure', new TypeError('wrong settings'));
			} else {
				logger.error(
					`worker failed unexpectedly [pid:${this.#pid}, code:${code}]`
				);

				this.emit('@failure', new Error(`[pid:${this.#pid}, code:${code}]`));
			}

			this.close();
		});

		this.#channel = new Channel({
			workerChannel: this.#workerChannel,
			pid: process.pid,
		});

		this.#appData = appData ?? ({} as WorkerAppData);

		// Listen for 'running' notification.
		this.#channel.once(String(process.pid), (event: Event) => {
			if (event === Event.WORKER_RUNNING) {
				logger.debug('worker process running [pid:${this.#pid}]');

				this.emit('@success');
			}
		});
	}

	get pid(): number {
		return this.#pid;
	}

	get closed(): boolean {
		return this.#closed;
	}

	get appData(): WorkerAppData {
		return this.#appData;
	}

	set appData(appData: WorkerAppData) {
		this.#appData = appData;
	}

	get observer(): WorkerObserver {
		return this.#observer;
	}

	/**
	 * Just for testing purposes.
	 */
	get webRtcServersForTesting(): Set<WebRtcServer> {
		return this.#webRtcServers;
	}

	/**
	 * Just for testing purposes.
	 */
	get routersForTesting(): Set<Router> {
		return this.#routers;
	}

	close(): void {
		if (this.#closed) {
			return;
		}

		logger.debug('close()');

		this.#closed = true;

		this.#channel.request(FbsRequest.Method.WORKER_CLOSE).catch(() => {});

		// Close the Channel instance.
		this.#channel.close();

		// Close the WorkerChannel instance.
		this.#workerChannel.close();

		// Close every Router.
		for (const router of this.#routers) {
			router.workerClosed();
		}
		this.#routers.clear();

		// Close every WebRtcServer.
		for (const webRtcServer of this.#webRtcServers) {
			webRtcServer.workerClosed();
		}
		this.#webRtcServers.clear();

		/* Send Request. */
		this.#channel
			.request(FbsRequest.Method.WORKER_CLOSE)
			.then(() => {
				// Close the Channel instance now.
				this.#channel.close();
			})
			.catch(error => {
				logger.error(
					'close() | worker process failed to process the close request:',
					error
				);

				// Close the Channel instance anyway.
				this.#channel.close();
			});

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	async dump(): Promise<WorkerDump> {
		logger.debug('dump()');

		// Send the request and wait for the response.
		const response = await this.#channel.request(FbsRequest.Method.WORKER_DUMP);

		/* Decode Response. */
		const dump = new FbsWorker.DumpResponse();

		response.body(dump);

		return parseWorkerDumpResponse(dump);
	}

	async getResourceUsage(): Promise<WorkerResourceUsage> {
		logger.debug('getResourceUsage()');

		const response = await this.#channel.request(
			FbsRequest.Method.WORKER_GET_RESOURCE_USAGE
		);

		/* Decode Response. */
		const resourceUsage = new FbsWorker.ResourceUsageResponse();

		response.body(resourceUsage);

		const ru = resourceUsage.unpack();

		return {
			ru_utime: Number(ru.ruUtime),
			ru_stime: Number(ru.ruStime),
			ru_maxrss: Number(ru.ruMaxrss),
			ru_ixrss: Number(ru.ruIxrss),
			ru_idrss: Number(ru.ruIdrss),
			ru_isrss: Number(ru.ruIsrss),
			ru_minflt: Number(ru.ruMinflt),
			ru_majflt: Number(ru.ruMajflt),
			ru_nswap: Number(ru.ruNswap),
			ru_inblock: Number(ru.ruInblock),
			ru_oublock: Number(ru.ruOublock),
			ru_msgsnd: Number(ru.ruMsgsnd),
			ru_msgrcv: Number(ru.ruMsgrcv),
			ru_nsignals: Number(ru.ruNsignals),
			ru_nvcsw: Number(ru.ruNvcsw),
			ru_nivcsw: Number(ru.ruNivcsw),
		};
	}

	async updateSettings({
		logLevel,
		logTags,
	}: WorkerUpdateableSettings<WorkerAppData> = {}): Promise<void> {
		logger.debug('updateSettings()');

		// Build the request.
		const requestOffset = new FbsWorker.UpdateSettingsRequestT(
			logLevel,
			logTags
		).pack(this.#channel.bufferBuilder);

		await this.#channel.request(
			FbsRequest.Method.WORKER_UPDATE_SETTINGS,
			FbsRequest.Body.Worker_UpdateSettingsRequest,
			requestOffset
		);
	}

	async createWebRtcServer<WebRtcServerAppData extends AppData = AppData>({
		listenInfos,
		appData,
	}: WebRtcServerOptions<WebRtcServerAppData>): Promise<
		WebRtcServer<WebRtcServerAppData>
	> {
		logger.debug('createWebRtcServer()');

		if (appData && typeof appData !== 'object') {
			throw new TypeError('if given, appData must be an object');
		}

		// Build the request.
		const fbsListenInfos: FbsTransport.ListenInfoT[] = [];

		for (const listenInfo of listenInfos) {
			fbsListenInfos.push(
				new FbsTransport.ListenInfoT(
					listenInfo.protocol === 'udp'
						? FbsTransportProtocol.UDP
						: FbsTransportProtocol.TCP,
					listenInfo.ip,
					listenInfo.announcedAddress ?? listenInfo.announcedIp,
					Boolean(listenInfo.exposeInternalIp),
					listenInfo.port,
					portRangeToFbs(listenInfo.portRange),
					socketFlagsToFbs(listenInfo.flags),
					listenInfo.sendBufferSize,
					listenInfo.recvBufferSize
				)
			);
		}

		const webRtcServerId = utils.generateUUIDv4();

		const createWebRtcServerRequestOffset =
			new FbsWorker.CreateWebRtcServerRequestT(
				webRtcServerId,
				fbsListenInfos
			).pack(this.#channel.bufferBuilder);

		await this.#channel.request(
			FbsRequest.Method.WORKER_CREATE_WEBRTCSERVER,
			FbsRequest.Body.Worker_CreateWebRtcServerRequest,
			createWebRtcServerRequestOffset
		);

		const webRtcServer: WebRtcServer<WebRtcServerAppData> =
			new WebRtcServerImpl({
				internal: { webRtcServerId },
				channel: this.#channel,
				appData,
			});

		this.#webRtcServers.add(webRtcServer);
		webRtcServer.on('@close', () => this.#webRtcServers.delete(webRtcServer));

		// Emit observer event.
		this.#observer.safeEmit('newwebrtcserver', webRtcServer);

		return webRtcServer;
	}

	async createRouter<RouterAppData extends AppData = AppData>({
		mediaCodecs,
		appData,
	}: RouterOptions<RouterAppData> = {}): Promise<Router<RouterAppData>> {
		logger.debug('createRouter()');

		if (appData && typeof appData !== 'object') {
			throw new TypeError('if given, appData must be an object');
		}

		// Clone given media codecs to not modify input data.
		const clonedMediaCodecs = utils.clone<
			RouterRtpCodecCapability[] | undefined
		>(mediaCodecs);

		// This may throw.
		const rtpCapabilities =
			ortc.generateRouterRtpCapabilities(clonedMediaCodecs);

		const routerId = utils.generateUUIDv4();

		// Get flatbuffer builder.
		const createRouterRequestOffset = new FbsWorker.CreateRouterRequestT(
			routerId
		).pack(this.#channel.bufferBuilder);

		await this.#channel.request(
			FbsRequest.Method.WORKER_CREATE_ROUTER,
			FbsRequest.Body.Worker_CreateRouterRequest,
			createRouterRequestOffset
		);

		const data = { rtpCapabilities };
		const router: Router<RouterAppData> = new RouterImpl({
			internal: {
				routerId,
			},
			data,
			channel: this.#channel,
			appData,
		});

		this.#routers.add(router);
		router.on('@close', () => this.#routers.delete(router));

		// Emit observer event.
		this.#observer.safeEmit('newrouter', router);

		return router;
	}
}

function parseWorkerDumpResponse(binary: FbsWorker.DumpResponse): WorkerDump {
	const dump: WorkerDump = {
		pid: binary.pid(),
		webRtcServerIds: fbsUtils.parseVector(binary, 'webRtcServerIds'),
		routerIds: fbsUtils.parseVector(binary, 'routerIds'),
		channelMessageHandlers: {
			channelRequestHandlers: fbsUtils.parseVector(
				binary.channelMessageHandlers()!,
				'channelRequestHandlers'
			),
			channelNotificationHandlers: fbsUtils.parseVector(
				binary.channelMessageHandlers()!,
				'channelNotificationHandlers'
			),
		},
	};

	if (binary.liburing()) {
		dump.liburing = {
			sqeProcessCount: Number(binary.liburing()!.sqeProcessCount()),
			sqeMissCount: Number(binary.liburing()!.sqeMissCount()),
			userDataMissCount: Number(binary.liburing()!.userDataMissCount()),
		};
	}

	return dump;
}

function getWorkerBin(): string {
	// If MEDIASOUP_WORKER_BIN env is given, use it as worker binary.
	if (process.env['MEDIASOUP_WORKER_BIN']) {
		logger.debug(
			`getWorkerBin() | using MEDIASOUP_WORKER_BIN environment variable: ${process.env['MEDIASOUP_WORKER_BIN']}`
		);

		return process.env['MEDIASOUP_WORKER_BIN'];
	}

	// Obtain the path of the mediasoup module.
	let mediasoupModulePath: string | undefined;

	try {
		// NOTE: This will throw `MODULE_NOT_FOUND` if mediasoup is installed
		// globally.
		mediasoupModulePath = require.resolve('mediasoup');

		// NOTE: Returned path will include 'node/lib/index.js' since that's the
		// main entry point in package.json, so remove it.
		mediasoupModulePath = path.join(
			path.dirname(mediasoupModulePath),
			'..',
			'..'
		);
	} catch (error) {
		logger.warn(
			`getWorkerBin() | require.resolve('mediasoup') failed, using __dirname: ${error}`
		);

		// mediasoup module path is two folders above this file.
		mediasoupModulePath = path.join(__dirname, '..', '..');
	}

	// If env MEDIASOUP_BUILDTYPE is 'Debug' use the Debug binary. Otherwise use
	// the Release binary.
	const buildType: 'Release' | 'Debug' =
		process.env['MEDIASOUP_BUILDTYPE'] === 'Debug' ? 'Debug' : 'Release';

	const workerBinPath = path.join(
		mediasoupModulePath,
		'worker',
		'out',
		buildType,
		'mediasoup-worker'
	);

	logger.debug(
		`getWorkerBin() | detected worker binary path: ${workerBinPath}`
	);

	return workerBinPath;
}
