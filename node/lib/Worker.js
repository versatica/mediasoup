"use strict";
var __classPrivateFieldSet = (this && this.__classPrivateFieldSet) || function (receiver, privateMap, value) {
    if (!privateMap.has(receiver)) {
        throw new TypeError("attempted to set private field on non-instance");
    }
    privateMap.set(receiver, value);
    return value;
};
var __classPrivateFieldGet = (this && this.__classPrivateFieldGet) || function (receiver, privateMap) {
    if (!privateMap.has(receiver)) {
        throw new TypeError("attempted to get private field on non-instance");
    }
    return privateMap.get(receiver);
};
var _child, _pid, _channel, _payloadChannel, _closed, _appData, _routers, _observer;
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
        ? path.join(__dirname, '..', '..', 'worker', 'out', 'Debug', 'mediasoup-worker')
        : path.join(__dirname, '..', '..', 'worker', 'out', 'Release', 'mediasoup-worker');
const logger = new Logger_1.Logger('Worker');
const workerLogger = new Logger_1.Logger('Worker');
class Worker extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     * @emits died - (error: Error)
     * @emits @success
     * @emits @failure - (error: Error)
     */
    constructor({ logLevel, logTags, rtcMinPort, rtcMaxPort, dtlsCertificateFile, dtlsPrivateKeyFile, appData }) {
        super();
        // mediasoup-worker child process.
        _child.set(this, void 0);
        // Worker process PID.
        _pid.set(this, void 0);
        // Channel instance.
        _channel.set(this, void 0);
        // PayloadChannel instance.
        _payloadChannel.set(this, void 0);
        // Closed flag.
        _closed.set(this, false);
        // Custom app data.
        _appData.set(this, void 0);
        // Routers set.
        _routers.set(this, new Set());
        // Observer instance.
        _observer.set(this, new EnhancedEventEmitter_1.EnhancedEventEmitter());
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
        __classPrivateFieldSet(this, _child, child_process_1.spawn(
        // command
        spawnBin, 
        // args
        spawnArgs, 
        // options
        {
            env: {
                MEDIASOUP_VERSION: '3.9.0',
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
        }));
        __classPrivateFieldSet(this, _pid, __classPrivateFieldGet(this, _child).pid);
        __classPrivateFieldSet(this, _channel, new Channel_1.Channel({
            producerSocket: __classPrivateFieldGet(this, _child).stdio[3],
            consumerSocket: __classPrivateFieldGet(this, _child).stdio[4],
            pid: __classPrivateFieldGet(this, _pid)
        }));
        __classPrivateFieldSet(this, _payloadChannel, new PayloadChannel_1.PayloadChannel({
            // NOTE: TypeScript does not like more than 5 fds.
            // @ts-ignore
            producerSocket: __classPrivateFieldGet(this, _child).stdio[5],
            // @ts-ignore
            consumerSocket: __classPrivateFieldGet(this, _child).stdio[6]
        }));
        __classPrivateFieldSet(this, _appData, appData);
        let spawnDone = false;
        // Listen for 'running' notification.
        __classPrivateFieldGet(this, _channel).once(String(__classPrivateFieldGet(this, _pid)), (event) => {
            if (!spawnDone && event === 'running') {
                spawnDone = true;
                logger.debug('worker process running [pid:%s]', __classPrivateFieldGet(this, _pid));
                this.emit('@success');
            }
        });
        __classPrivateFieldGet(this, _child).on('exit', (code, signal) => {
            __classPrivateFieldSet(this, _child, undefined);
            this.close();
            if (!spawnDone) {
                spawnDone = true;
                if (code === 42) {
                    logger.error('worker process failed due to wrong settings [pid:%s]', __classPrivateFieldGet(this, _pid));
                    this.emit('@failure', new TypeError('wrong settings'));
                }
                else {
                    logger.error('worker process failed unexpectedly [pid:%s, code:%s, signal:%s]', __classPrivateFieldGet(this, _pid), code, signal);
                    this.emit('@failure', new Error(`[pid:${__classPrivateFieldGet(this, _pid)}, code:${code}, signal:${signal}]`));
                }
            }
            else {
                logger.error('worker process died unexpectedly [pid:%s, code:%s, signal:%s]', __classPrivateFieldGet(this, _pid), code, signal);
                this.safeEmit('died', new Error(`[pid:${__classPrivateFieldGet(this, _pid)}, code:${code}, signal:${signal}]`));
            }
        });
        __classPrivateFieldGet(this, _child).on('error', (error) => {
            __classPrivateFieldSet(this, _child, undefined);
            this.close();
            if (!spawnDone) {
                spawnDone = true;
                logger.error('worker process failed [pid:%s]: %s', __classPrivateFieldGet(this, _pid), error.message);
                this.emit('@failure', error);
            }
            else {
                logger.error('worker process error [pid:%s]: %s', __classPrivateFieldGet(this, _pid), error.message);
                this.safeEmit('died', error);
            }
        });
        // Be ready for 3rd party worker libraries logging to stdout.
        __classPrivateFieldGet(this, _child).stdout.on('data', (buffer) => {
            for (const line of buffer.toString('utf8').split('\n')) {
                if (line)
                    workerLogger.debug(`(stdout) ${line}`);
            }
        });
        // In case of a worker bug, mediasoup will log to stderr.
        __classPrivateFieldGet(this, _child).stderr.on('data', (buffer) => {
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
        return __classPrivateFieldGet(this, _pid);
    }
    /**
     * Whether the Worker is closed.
     */
    get closed() {
        return __classPrivateFieldGet(this, _closed);
    }
    /**
     * App custom data.
     */
    get appData() {
        return __classPrivateFieldGet(this, _appData);
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
        return __classPrivateFieldGet(this, _observer);
    }
    /**
     * @private
     * Just for testing purposes.
     */
    get routersForTesting() {
        return __classPrivateFieldGet(this, _routers);
    }
    /**
     * Close the Worker.
     */
    close() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('close()');
        __classPrivateFieldSet(this, _closed, true);
        // Kill the worker process.
        if (__classPrivateFieldGet(this, _child)) {
            // Remove event listeners but leave a fake 'error' hander to avoid
            // propagation.
            __classPrivateFieldGet(this, _child).removeAllListeners('exit');
            __classPrivateFieldGet(this, _child).removeAllListeners('error');
            __classPrivateFieldGet(this, _child).on('error', () => { });
            __classPrivateFieldGet(this, _child).kill('SIGTERM');
            __classPrivateFieldSet(this, _child, undefined);
        }
        // Close the Channel instance.
        __classPrivateFieldGet(this, _channel).close();
        // Close the PayloadChannel instance.
        __classPrivateFieldGet(this, _payloadChannel).close();
        // Close every Router.
        for (const router of __classPrivateFieldGet(this, _routers)) {
            router.workerClosed();
        }
        __classPrivateFieldGet(this, _routers).clear();
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('close');
    }
    /**
     * Dump Worker.
     */
    async dump() {
        logger.debug('dump()');
        return __classPrivateFieldGet(this, _channel).request('worker.dump');
    }
    /**
     * Get mediasoup-worker process resource usage.
     */
    async getResourceUsage() {
        logger.debug('getResourceUsage()');
        return __classPrivateFieldGet(this, _channel).request('worker.getResourceUsage');
    }
    /**
     * Update settings.
     */
    async updateSettings({ logLevel, logTags } = {}) {
        logger.debug('updateSettings()');
        const reqData = { logLevel, logTags };
        await __classPrivateFieldGet(this, _channel).request('worker.updateSettings', undefined, reqData);
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
        await __classPrivateFieldGet(this, _channel).request('worker.createRouter', internal);
        const data = { rtpCapabilities };
        const router = new Router_1.Router({
            internal,
            data,
            channel: __classPrivateFieldGet(this, _channel),
            payloadChannel: __classPrivateFieldGet(this, _payloadChannel),
            appData
        });
        __classPrivateFieldGet(this, _routers).add(router);
        router.on('@close', () => __classPrivateFieldGet(this, _routers).delete(router));
        // Emit observer event.
        __classPrivateFieldGet(this, _observer).safeEmit('newrouter', router);
        return router;
    }
}
exports.Worker = Worker;
_child = new WeakMap(), _pid = new WeakMap(), _channel = new WeakMap(), _payloadChannel = new WeakMap(), _closed = new WeakMap(), _appData = new WeakMap(), _routers = new WeakMap(), _observer = new WeakMap();
