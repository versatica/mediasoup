import * as process from 'node:process';
import * as path from 'node:path';
import type { Duplex } from 'node:stream';
import { spawn, ChildProcess } from 'node:child_process';
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
import * as FbsNotification from './fbs/notification';
import * as FbsRequest from './fbs/request';
import * as FbsWorker from './fbs/worker';
import * as FbsTransport from './fbs/transport';
import { Protocol as FbsTransportProtocol } from './fbs/transport/protocol';

const logger = new Logger('Worker');
const workerLogger = new Logger('Worker');

export const defaultWorkerBin: string = getDefaultWorkerBin();

export class WorkerImpl<WorkerAppData extends AppData = AppData>
	extends EnhancedEventEmitter<WorkerEvents>
	implements Worker
{
	// mediasoup-worker child process.
	#child: ChildProcess;

	// Worker process PID.
	readonly #pid: number;

	// Channel instance.
	readonly #channel: Channel;

	// Closed flag.
	#closed = false;

	// Died dlag.
	#died = false;

	// Worker process closed flag.
	#subprocessClosed = false;

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
		workerBin,
		libwebrtcFieldTrials,
		disableLiburing,
		appData,
	}: WorkerSettings<WorkerAppData>) {
		super();

		logger.debug('constructor()');

		workerBin = workerBin ?? defaultWorkerBin;

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

		this.#child = spawn(
			// command
			spawnBin,
			// args
			spawnArgs,
			// options
			{
				env: {
					MEDIASOUP_VERSION: version,
					// Let the worker process inherit all environment variables, useful
					// if a custom and not in the path GCC is used so the user can set
					// LD_LIBRARY_PATH environment variable for runtime.
					...process.env,
				},

				detached: false,

				// fd 0 (stdin)   : Just ignore it.
				// fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
				// fd 2 (stderr)  : Same as stdout.
				// fd 3 (channel) : Producer Channel fd.
				// fd 4 (channel) : Consumer Channel fd.
				stdio: ['ignore', 'pipe', 'pipe', 'pipe', 'pipe'],
				windowsHide: true,
			}
		);

		this.#pid = this.#child.pid!;

		this.#channel = new Channel({
			producerSocket: this.#child.stdio[3] as Duplex,
			consumerSocket: this.#child.stdio[4] as Duplex,
			pid: this.#pid,
		});

		this.#appData = appData ?? ({} as WorkerAppData);

		let spawnDone = false;

		// Listen for 'running' notification.
		this.#channel.once(String(this.#pid), (event: Event) => {
			if (!spawnDone && event === Event.WORKER_RUNNING) {
				spawnDone = true;

				logger.debug(`worker process running [pid:${this.#pid}]`);

				this.emit('@success');
			}
		});

		this.#child.on('exit', (code, signal) => {
			// If closed by ourselves, do nothing.
			if (this.#closed) {
				return;
			}

			if (!spawnDone) {
				spawnDone = true;

				if (code === 42) {
					logger.error(
						`worker process failed due to wrong settings [pid:${this.#pid}]`
					);

					this.close();
					this.emit('@failure', new TypeError('wrong settings'));
				} else {
					logger.error(
						`worker process failed unexpectedly [pid:${this.#pid}, code:${code}, signal:${signal}]`
					);

					this.close();
					this.emit(
						'@failure',
						new Error(`[pid:${this.#pid}, code:${code}, signal:${signal}]`)
					);
				}
			} else {
				logger.error(
					`worker process died unexpectedly [pid:${this.#pid}, code:${code}, signal:${signal}]`
				);

				this.workerDied(
					new Error(`[pid:${this.#pid}, code:${code}, signal:${signal}]`)
				);
			}
		});

		this.#child.on('error', error => {
			// If closed by ourselves, do nothing.
			if (this.#closed) {
				return;
			}

			if (!spawnDone) {
				spawnDone = true;

				logger.error(
					`worker process failed [pid:${this.#pid}]: ${error.message}`
				);

				this.close();
				this.emit('@failure', new Error(error.message));
			} else {
				logger.error(
					`worker process error [pid:${this.#pid}]: ${error.message}`
				);

				this.workerDied(error);
			}
		});

		this.#child.on('close', (code, signal) => {
			logger.debug(
				`worker process closed [pid:${this.#pid}, code:${code}, signal:${signal}]`
			);

			if (!this.#subprocessClosed) {
				this.#subprocessClosed = true;

				logger.debug(`emitting 'subprocessclose' event`);

				this.safeEmit('subprocessclose');
			}
		});

		// Be ready for 3rd party worker libraries logging to stdout.
		this.#child.stdout!.on('data', buffer => {
			for (const line of buffer.toString('utf8').split('\n')) {
				if (line) {
					workerLogger.debug(`(stdout) ${line}`);
				}
			}
		});

		// In case of a worker bug, mediasoup will log to stderr.
		this.#child.stderr!.on('data', buffer => {
			for (const line of buffer.toString('utf8').split('\n')) {
				if (line) {
					workerLogger.error(`(stderr) ${line}`);
				}
			}
		});

		this.handleListenerError();
	}

	get pid(): number {
		return this.#pid;
	}

	get closed(): boolean {
		return this.#closed;
	}

	get died(): boolean {
		return this.#died;
	}

	get subprocessClosed(): boolean {
		return this.#subprocessClosed;
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

		// Send notification to worker process.
		this.#channel.notify(FbsNotification.Event.WORKER_CLOSE);

		// Close the Channel instance now.
		this.#channel.close();

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

	private workerDied(error: Error): void {
		if (this.#closed) {
			return;
		}

		logger.debug(`workerDied() [error:${error.toString()}]`);

		this.#closed = true;
		this.#subprocessClosed = true;
		this.#died = true;

		// Close the Channel instance.
		this.#channel.close();

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

		logger.debug(`workerDied() | emitting 'died' and 'subprocessclose' events`);

		this.safeEmit('died', error);
		this.safeEmit('subprocessclose');

		// Emit observer event.
		this.#observer.safeEmit('close');
	}

	private handleListenerError(): void {
		this.on('listenererror', (eventName, error) => {
			logger.error(
				`event listener threw an error [eventName:${eventName}]:`,
				error
			);
		});
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

function getDefaultWorkerBin(): string {
	// If MEDIASOUP_WORKER_BIN env is given, use it as worker binary.
	if (process.env['MEDIASOUP_WORKER_BIN']) {
		logger.debug(
			`getDefaultWorkerBin() | using MEDIASOUP_WORKER_BIN environment variable: ${process.env['MEDIASOUP_WORKER_BIN']}`
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
			`getDefaultWorkerBin() | require.resolve('mediasoup') failed, using __dirname: ${error}`
		);

		// mediasoup module path is two folders above this file.
		mediasoupModulePath = path.join(__dirname, '..', '..');
	}

	// If env MEDIASOUP_BUILDTYPE is 'Debug' use the Debug binary. Otherwise use
	// the Release binary.
	const buildType: 'Release' | 'Debug' =
		process.env['MEDIASOUP_BUILDTYPE'] === 'Debug' ? 'Debug' : 'Release';

	const defaultWorkerBinPath = path.join(
		mediasoupModulePath,
		'worker',
		'out',
		buildType,
		'mediasoup-worker'
	);

	logger.debug(
		`getDefaultWorkerBin() | detected worker binary path: ${defaultWorkerBinPath}`
	);

	return defaultWorkerBinPath;
}
