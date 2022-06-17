"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.WebRtcServer = void 0;
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
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
     * @emits workerclose
     * @emits @close
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
     *
     * @emits close
     * @emits newwebrtctransport - (webRtcTransport: WebRtcTransport)
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
        this.#channel.request('webRtcServer.close', this.#internal)
            .catch(() => { });
        // Close every WebRtcTransport.
        for (const webRtcTransport of this.#webRtcTransports.values()) {
            webRtcTransport.mustClose();
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
        // Close every WebRtcTransport.
        for (const webRtcTransport of this.#webRtcTransports.values()) {
            webRtcTransport.mustClose();
        }
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
        return this.#channel.request('webRtcServer.dump', this.#internal);
    }
    /**
     * @private
     */
    handleWebRtcTransport(webRtcTransport) {
        this.#webRtcTransports.set(webRtcTransport.id, webRtcTransport);
        webRtcTransport.on('@close', () => this.#webRtcTransports.delete(webRtcTransport.id));
        // Emit observer event.
        this.#observer.safeEmit('newwebrtctransport', webRtcTransport);
    }
}
exports.WebRtcServer = WebRtcServer;
