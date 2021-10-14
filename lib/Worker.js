"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
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
        ? path.join(__dirname, '..', 'worker', 'out', 'Debug', 'mediasoup-worker')
        : path.join(__dirname, '..', 'worker', 'out', 'Release', 'mediasoup-worker');
const logger = new Logger_1.Logger('Worker');
const workerLogger = new Logger_1.Logger('Worker');
class Worker extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     * @emits died - (error: Error)
     * @emits @success
     * @emits @failure - (error: Error)
     */
    constructor({ logLevel, logTags, logDevLevel, logTraceEnabled, logFile, rtcMinPort, rtcMaxPort, dtlsCertificateFile, dtlsPrivateKeyFile, appData }) {
        super();
        // Closed flag.
        this._closed = false;
        // Routers set.
        this._routers = new Set();
        // Observer instance.
        this._observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
        logger.debug('constructor()');
        let spawnBin = workerBin;
        let spawnArgs = [];
        if (process.env.MEDIASOUP_USE_VALGRIND === 'true') {
            spawnBin = process.env.MEDIASOUP_VALGRIND_BIN || 'valgrind';
            if (process.env.MEDIASOUP_VALGRIND_OPTIONS)
                spawnArgs = spawnArgs.concat(process.env.MEDIASOUP_VALGRIND_OPTIONS.split(/\s+/));
            spawnArgs.push(workerBin);
        }
        if (typeof logLevel === 'string' && logLevel)
            spawnArgs.push(`--logLevel=${logLevel}`);
        for (const logTag of (Array.isArray(logTags) ? logTags : [])) {
            if (typeof logTag === 'string' && logTag)
                spawnArgs.push(`--logTag=${logTag}`);
        }
        if (typeof logDevLevel === 'string' && logDevLevel)
            spawnArgs.push(`--logDevLevel=${logDevLevel}`);
        if (typeof logTraceEnabled === 'boolean' && logTraceEnabled && logTraceEnabled === true)
            spawnArgs.push(`--logTraceEnabled=true`);
        if (typeof rtcMinPort === 'number' && !Number.isNaN(rtcMinPort))
            spawnArgs.push(`--rtcMinPort=${rtcMinPort}`);
        if (typeof rtcMaxPort === 'number' && !Number.isNaN(rtcMaxPort))
            spawnArgs.push(`--rtcMaxPort=${rtcMaxPort}`);
        if (typeof dtlsCertificateFile === 'string' && dtlsCertificateFile)
            spawnArgs.push(`--dtlsCertificateFile=${dtlsCertificateFile}`);
        if (typeof dtlsPrivateKeyFile === 'string' && dtlsPrivateKeyFile)
            spawnArgs.push(`--dtlsPrivateKeyFile=${dtlsPrivateKeyFile}`);
        logger.debug('spawning worker process: %s %s', spawnBin, spawnArgs.join(' '));
        this._child = child_process_1.spawn(
        // command
        spawnBin, 
        // args
        spawnArgs, 
        // options
        {
            env: {
                MEDIASOUP_VERSION: '3.8.3-lv7'
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
        this._pid = this._child.pid;
        // each worker writes into its own log file: insert pid before file extension
        this._mslog = path.format({
            dir: path.dirname(logFile),
            name: path.basename(logFile, path.extname(logFile)) + "." + this._pid,
            ext: path.extname(logFile)
        });
        this._channel = new Channel_1.Channel({
            producerSocket: this._child.stdio[3],
            consumerSocket: this._child.stdio[4],
            pid: this._pid
        });
        this._payloadChannel = new PayloadChannel_1.PayloadChannel({
            // NOTE: TypeScript does not like more than 5 fds.
            // @ts-ignore
            producerSocket: this._child.stdio[5],
            // @ts-ignore
            consumerSocket: this._child.stdio[6]
        });
        this._appData = appData;
        let spawnDone = false;
        this._channel.on(String(this._pid), (event, data) => {
            // Listen for 'running' notification.
            if (!spawnDone && event === 'running') {
                spawnDone = true;
                logger.debug('worker process running [pid:%s][log:%s]', this._pid, this._mslog);
                this.emit('@success');
                // Tell C++ worker to begin logging. All workers share a single log file.
                this.logOpen();
            }
            // Once worker started begin listening for issues with logging
            else if (spawnDone) {
                if (event === 'failedlog') {
                    const logerr = data;
                    this.safeEmit('failedlog', logerr);
                    // Emit observer event.
                    this._observer.safeEmit('failedlog', logerr);
                }
            }
        });
        this._child.on('exit', (code, signal) => {
            this._child = undefined;
            this.close();
            if (!spawnDone) {
                spawnDone = true;
                if (code === 42) {
                    logger.error('worker process failed due to wrong settings [pid:%s]', this._pid);
                    this.emit('@failure', new TypeError('wrong settings'));
                }
                else {
                    logger.error('worker process failed unexpectedly [pid:%s, code:%s, signal:%s]', this._pid, code, signal);
                    this.emit('@failure', new Error(`[pid:${this._pid}, code:${code}, signal:${signal}]`));
                }
            }
            else {
                logger.error('worker process died unexpectedly [pid:%s, code:%s, signal:%s]', this._pid, code, signal);
                this.safeEmit('died', new Error(`[pid:${this._pid}, code:${code}, signal:${signal}]`));
            }
        });
        this._child.on('error', (error) => {
            this._child = undefined;
            this.close();
            if (!spawnDone) {
                spawnDone = true;
                logger.error('worker process failed [pid:%s]: %s', this._pid, error.message);
                this.emit('@failure', error);
            }
            else {
                logger.error('worker process error [pid:%s]: %s', this._pid, error.message);
                this.safeEmit('died', error);
            }
        });
        // Be ready for 3rd party worker libraries logging to stdout.
        this._child.stdout.on('data', (buffer) => {
            for (const line of buffer.toString('utf8').split('\n')) {
                if (line)
                    workerLogger.debug(`(stdout) ${line}`);
            }
        });
        // In case of a worker bug, mediasoup will log to stderr.
        this._child.stderr.on('data', (buffer) => {
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
        return this._pid;
    }
    /**
     * Whether the Worker is closed.
     */
    get closed() {
        return this._closed;
    }
    /**
     * App custom data.
     */
    get appData() {
        return this._appData;
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
        return this._observer;
    }
    /**
     * Close the Worker.
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        // Kill the worker process.
        if (this._child) {
            // Remove event listeners but leave a fake 'error' hander to avoid
            // propagation.
            this._child.removeAllListeners('exit');
            this._child.removeAllListeners('error');
            this._child.on('error', () => { });
            this._child.kill('SIGTERM');
            this._child = undefined;
        }
        // Close the Channel instance.
        this._channel.close();
        // Close the PayloadChannel instance.
        this._payloadChannel.close();
        // Close every Router.
        for (const router of this._routers) {
            router.workerClosed();
        }
        this._routers.clear();
        // Emit observer event.
        this._observer.safeEmit('close');
    }
    /**
     * Dump Worker.
     */
    async dump() {
        logger.debug('dump()');
        return this._channel.request('worker.dump');
    }
    /**
     * Get mediasoup-worker process resource usage.
     */
    async getResourceUsage() {
        logger.debug('getResourceUsage()');
        return this._channel.request('worker.getResourceUsage');
    }
    /**
     * Open specified log file
    **/
    async logOpen() {
        logger.debug('logOpen(): %s', this._mslog);
        const reqData = { mslogname: this._mslog };
        await this._channel.request('worker.logopen', undefined, reqData);
    }
    /**
     * Reopen same log file by name after logrotate
    **/
    async logRotate() {
        logger.debug('logRotate()');
        await this._channel.request('worker.logrotate');
    }
    /**
     * Update settings.
     */
    async updateSettings({ logLevel, logTags, logDevLevel, logTraceEnabled, } = {}) {
        logger.debug('updateSettings()');
        const reqData = { logLevel, logTags, logDevLevel, logTraceEnabled };
        await this._channel.request('worker.updateSettings', undefined, reqData);
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
        const internal = { routerId: uuid_1.v4() };
        await this._channel.request('worker.createRouter', internal);
        const data = { rtpCapabilities };
        const router = new Router_1.Router({
            internal,
            data,
            channel: this._channel,
            payloadChannel: this._payloadChannel,
            appData
        });
        this._routers.add(router);
        router.on('@close', () => this._routers.delete(router));
        // Emit observer event.
        this._observer.safeEmit('newrouter', router);
        return router;
    }
}
exports.Worker = Worker;
