"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.WebRtcServer = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const request_generated_1 = require("./fbs/request_generated");
const webrtcserver_generated_1 = require("./fbs/webrtcserver_generated");
const logger = new Logger_1.Logger('WebRtcServer');
class WebRtcServer extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    // Internal data.
    #internal;
    // Channel instance.
    #channel;
    // Closed flag.
    #closed = false;
    // Custom app data.
    #appData;
    // Transports map.
    #webRtcTransports = new Map();
    // Observer instance.
    #observer = new EnhancedEventEmitter_1.EnhancedEventEmitter();
    /**
     * @private
     */
    constructor({ internal, channel, appData }) {
        super();
        logger.debug('constructor()');
        this.#internal = internal;
        this.#channel = channel;
        this.#appData = appData || {};
    }
    /**
     * WebRtcServer id.
     */
    get id() {
        return this.#internal.webRtcServerId;
    }
    /**
     * Whether the WebRtcServer is closed.
     */
    get closed() {
        return this.#closed;
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
     */
    get observer() {
        return this.#observer;
    }
    /**
     * @private
     * Just for testing purposes.
     */
    get webRtcTransportsForTesting() {
        return this.#webRtcTransports;
    }
    /**
     * Close the WebRtcServer.
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Build the request.
        const builder = this.#channel.bufferBuilder;
        const bodyOffset = new request_generated_1.CloseWebRtcServerRequestT(this.#internal.webRtcServerId).pack(builder);
        this.#channel.requestBinary(request_generated_1.Method.WORKER_WEBRTC_SERVER_CLOSE, request_generated_1.Body.FBS_Worker_CloseWebRtcServerRequest, bodyOffset)
            .catch(() => { });
        // Close every WebRtcTransport.
        for (const webRtcTransport of this.#webRtcTransports.values()) {
            webRtcTransport.listenServerClosed();
            // Emit observer event.
            this.#observer.safeEmit('webrtctransportunhandled', webRtcTransport);
        }
        this.#webRtcTransports.clear();
        this.emit('@close');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Worker was closed.
     *
     * @private
     */
    workerClosed() {
        if (this.#closed)
            return;
        logger.debug('workerClosed()');
        this.#closed = true;
        // NOTE: No need to close WebRtcTransports since they are closed by their
        // respective Router parents.
        this.#webRtcTransports.clear();
        this.safeEmit('workerclose');
        // Emit observer event.
        this.#observer.safeEmit('close');
    }
    /**
     * Dump WebRtcServer.
     */
    async dump() {
        logger.debug('dump()');
        const response = await this.#channel.requestBinary(request_generated_1.Method.WEBRTC_SERVER_DUMP, undefined, undefined, this.#internal.webRtcServerId);
        /* Decode the response. */
        const dump = new webrtcserver_generated_1.WebRtcServerDump();
        response.body(dump);
        return dump.unpack();
    }
    /**
     * @private
     */
    handleWebRtcTransport(webRtcTransport) {
        this.#webRtcTransports.set(webRtcTransport.id, webRtcTransport);
        // Emit observer event.
        this.#observer.safeEmit('webrtctransporthandled', webRtcTransport);
        webRtcTransport.on('@close', () => {
            this.#webRtcTransports.delete(webRtcTransport.id);
            // Emit observer event.
            this.#observer.safeEmit('webrtctransportunhandled', webRtcTransport);
        });
    }
}
exports.WebRtcServer = WebRtcServer;
