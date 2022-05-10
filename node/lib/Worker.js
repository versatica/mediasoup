"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.Worker = void 0;
const process = require("process");
const path = require("path");
const child_process_1 = require("child_process");
const uuid_1 = require("uuid");
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const ortc = require("./ortc");
const Channel_1 = require("./Channel");
const PayloadChannel_1 = require("./PayloadChannel");
const Router_1 = require("./Router");
// If env MEDIASOUP_WORKER_BIN is given, use it as worker binary.
// Otherwise if env MEDIASOUP_BUILDTYPE is 'Debug' use the Debug binary.
// Otherwise use the Release binary.
const workerBin = process.env.MEDIASOUP_WORKER_BIN
    ? process.env.MEDIASOUP_WORKER_BIN
    : process.env.MEDIASOUP_BUILDTYPE === 'Debug'
        ? path.join(__dirname, '..', '..', 'worker', 'out', 'Debug', 'mediasoup-worker')
        : path.join(__dirname, '..', '..', 'worker', 'out', 'Release', 'mediasoup-worker');
const logger = new Logger_1.Logger('Worker');
const workerLogger = new Logger_1.Logger('Worker');
class Worker extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    // mediasoup-worker child process.
    #child;
    // Worker process PID.
    #pid;
    // Channel instance.
    #channel;
    // PayloadChannel instance.
    #payloadChannel;
    // Closed flag.
    #closed = false;
    // Died dlag.
    #died = false;
    // Custom app data.
    #appData;
    // Routers set.
    #routers = new Set();
    // Observer instance.
    #observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
    /**
     * @private
     * @emits died - (error: Error)
     * @emits @success
     * @emits @failure - (error: Error)
     */
    constructor({ logLevel, logTags, rtcMinPort, rtcMaxPort, dtlsCertificateFile, dtlsPrivateKeyFile, appData }) {
        super();
        logger.debug('constructor()');
        let spawnBin = workerBin;
        let spawnArgs = [];
        if (process.env.MEDIASOUP_USE_VALGRIND === 'true') {
            spawnBin = process.env.MEDIASOUP_VALGRIND_BIN || 'valgrind';
            if (process.env.MEDIASOUP_VALGRIND_OPTIONS) {
                spawnArgs = spawnArgs.concat(process.env.MEDIASOUP_VALGRIND_OPTIONS.split(/\s+/));
            }
            spawnArgs.push(workerBin);
        }
        if (typeof logLevel === 'string' && logLevel)
            spawnArgs.push(`--logLevel=${logLevel}`);
        for (const logTag of (Array.isArray(logTags) ? logTags : [])) {
            if (typeof logTag === 'string' && logTag)
                spawnArgs.push(`--logTag=${logTag}`);
        }
        if (typeof rtcMinPort === 'number' && !Number.isNaN(rtcMinPort))
            spawnArgs.push(`--rtcMinPort=${rtcMinPort}`);
        if (typeof rtcMaxPort === 'number' && !Number.isNaN(rtcMaxPort))
            spawnArgs.push(`--rtcMaxPort=${rtcMaxPort}`);
        if (typeof dtlsCertificateFile === 'string' && dtlsCertificateFile)
            spawnArgs.push(`--dtlsCertificateFile=${dtlsCertificateFile}`);
        if (typeof dtlsPrivateKeyFile === 'string' && dtlsPrivateKeyFile)
            spawnArgs.push(`--dtlsPrivateKeyFile=${dtlsPrivateKeyFile}`);
        logger.debug('spawning worker process: %s %s', spawnBin, spawnArgs.join(' '));
        this.#child = (0, child_process_1.spawn)(
        // command
        spawnBin, 
        // args
        spawnArgs, 
        // options
        {
            env: {
                MEDIASOUP_VERSION: '3.9.13',
                // Let the worker process inherit all environment variables, useful
                // if a custom and not in the path GCC is used so the user can set
                // LD_LIBRARY_PATH environment variable for runtime.
                ...process.env
            },
            detached: false,
            // fd 0 (stdin)   : Just ignore it.
            // fd 1 (stdout)  : Pipe it for 3rd libraries that log their own stuff.
            // fd 2 (stderr)  : Same as stdout.
            // fd 3 (channel) : Producer Channel fd.
            // fd 4 (channel) : Consumer Channel fd.
            // fd 5 (channel) : Producer PayloadChannel fd.
            // fd 6 (channel) : Consumer PayloadChannel fd.
            stdio: ['ignore', 'pipe', 'pipe', 'pipe', 'pipe', 'pipe', 'pipe'],
            windowsHide: true
        });
        this.#pid = this.#child.pid;
        this.#channel = new Channel_1.Channel({
            producerSocket: this.#child.stdio[3],
            consumerSocket: this.#child.stdio[4],
            pid: this.#pid
        });
        this.#payloadChannel = new PayloadChannel_1.PayloadChannel({
            // NOTE: TypeScript does not like more than 5 fds.
            // @ts-ignore
            producerSocket: this.#child.stdio[5],
            // @ts-ignore
            consumerSocket: this.#child.stdio[6]
        });
        this.#appData = appData;
        let spawnDone = false;
        // Listen for 'running' notification.
        this.#channel.once(String(this.#pid), (event) => {
            if (!spawnDone && event === 'running') {
                spawnDone = true;
                logger.debug('worker process running [pid:%s]', this.#pid);
                this.emit('@success');
            }
        });
        this.#child.on('exit', (code, signal) => {
            this.#child = undefined;
            if (!spawnDone) {
                spawnDone = true;
                if (code === 42) {
                    logger.error('worker process failed due to wrong settings [pid:%s]', this.#pid);
                    this.close();
                    this.emit('@failure', new TypeError('wrong settings'));
                }
                else {
                    logger.error('worker process failed unexpectedly [pid:%s, code:%s, signal:%s]', this.#pid, code, signal);
                    this.close();
                    this.emit('@failure', new Error(`[pid:${this.#pid}, code:${code}, signal:${signal}]`));
                }
            }
            else {
                logger.error('worker process died unexpectedly [pid:%s, code:%s, signal:%s]', this.#pid, code, signal);
                this.workerDied(new Error(`[pid:${this.#pid}, code:${code}, signal:${signal}]`));
            }
        });
        this.#child.on('error', (error) => {
            this.#child = undefined;
            if (!spawnDone) {
                spawnDone = true;
                logger.error('worker process failed [pid:%s]: %s', this.#pid, error.message);
                this.close();
                this.emit('@failure', error);
            }
            else {
                logger.error('worker process error [pid:%s]: %s', this.#pid, error.message);
                this.workerDied(error);
            }
        });
        // Be ready for 3rd party worker libraries logging to stdout.
        this.#child.stdout.on('data', (buffer) => {
            for (const line of buffer.toString('utf8').split('\n')) {
                if (line)
                    workerLogger.debug(`(stdout) ${line}`);
            }
        });
        // In case of a worker bug, mediasoup will log to stderr.
        this.#child.stderr.on('data', (buffer) => {
            for (const line of buffer.toString('utf8').split('\n')) {
                if (line)
                    workerLogger.error(`(stderr) ${line}`);
            }
        });
    }
    /**
     * Worker process identifier (PID).
     */
    get pid() {
        return this.#pid;
    }
    /**
     * Whether the Worker is closed.
     */
    get closed() {
        return this.#closed;
    }
    /**
     * Whether the Worker died.
     */
    get died() {
        return this.#died;
    }
    /**
     * App custom data.
     */
    get appData() {
        return this.#appData;
    }
    /**
     * Invalid setter.
     */
    set appData(appData) {
        throw new Error('cannot override appData object');
    }
    /**
     * Observer.
     *
     * @emits close
     * @emits newrouter - (router: Router)
     */
    get observer() {
        return this.#observer;
    }
    /**
     * @private
     * Just for testing purposes.
     */
    get routersForTesting() {
        return this.#routers;
    }
    /**
     * Close the Worker.
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Kill the worker process.
        if (this.#child) {
            // Remove event listeners but leave a fake 'error' hander to avoid
            // propagation.
            this.#child.removeAllListeners('exit');
            this.#child.removeAllListeners('error');
            this.#child.on('error', () => { });
            this.#child.kill('SIGTERM');
            this.#child = undefined;
        }
        // Close the Channel instance.
        this.#channel.close();
        // Close the PayloadChannel instance.
        this.#payloadChannel.close();
        // Close every Router.
        for (const router of this.#routers) {
            router.workerClosed();
        }
        this.#routers.clear();
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Dump Worker.
     */
    async dump() {
        logger.debug('dump()');
        return this.#channel.request('worker.dump');
    }
    /**
     * Get mediasoup-worker process resource usage.
     */
    async getResourceUsage() {
        logger.debug('getResourceUsage()');
        return this.#channel.request('worker.getResourceUsage');
    }
    /**
     * Update settings.
     */
    async updateSettings({ logLevel, logTags } = {}) {
        logger.debug('updateSettings()');
        const reqData = { logLevel, logTags };
        await this.#channel.request('worker.updateSettings', undefined, reqData);
    }
    /**
     * Create a Router.
     */
    async createRouter({ mediaCodecs, appData = {} } = {}) {
        logger.debug('createRouter()');
        if (appData && typeof appData !== 'object')
            throw new TypeError('if given, appData must be an object');
        // This may throw.
        const rtpCapabilities = ortc.generateRouterRtpCapabilities(mediaCodecs);
        const internal = { routerId: (0, uuid_1.v4)() };
        await this.#channel.request('worker.createRouter', internal);
        const data = { rtpCapabilities };
        const router = new Router_1.Router({
            internal,
            data,
            channel: this.#channel,
            payloadChannel: this.#payloadChannel,
            appData
        });
        this.#routers.add(router);
        router.on('@close', () => this.#routers.delete(router));
        // Emit observer event.
        this.#observer.safeEmit('newrouter', router);
        return router;
    }
    workerDied(error) {
        if (this.#closed)
            return;
        logger.debug(`died() [error:${error}]`);
        this.#closed = true;
        this.#died = true;
        // Close the Channel instance.
        this.#channel.close();
        // Close the PayloadChannel instance.
        this.#payloadChannel.close();
        // Close every Router.
        for (const router of this.#routers) {
            router.workerClosed();
        }
        this.#routers.clear();
        this.safeEmit('died', error);
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
}
exports.Worker = Worker;
