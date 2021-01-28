"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
// @ts-ignore
const netstring = require("netstring");
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const errors_1 = require("./errors");
const logger = new Logger_1.Logger('Channel');
// netstring length for a 4194304 bytes payload.
const NS_MESSAGE_MAX_LEN = 4194313;
const NS_PAYLOAD_MAX_LEN = 4194304;
class Channel extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     */
    constructor({ producerSocket, consumerSocket, pid }) {
        super();
        // Closed flag.
        this._closed = false;
        // Next id for messages sent to the worker process.
        this._nextId = 0;
        // Map of pending sent requests.
        this._sents = new Map();
        logger.debug('constructor()');
        this._producerSocket = producerSocket;
        this._consumerSocket = consumerSocket;
        // Read Channel responses/notifications from the worker.
        this._consumerSocket.on('data', (buffer) => {
            if (!this._recvBuffer) {
                this._recvBuffer = buffer;
            }
            else {
                this._recvBuffer = Buffer.concat([this._recvBuffer, buffer], this._recvBuffer.length + buffer.length);
            }
            if (this._recvBuffer.length > NS_PAYLOAD_MAX_LEN) {
                logger.error('receiving buffer is full, discarding all data into it');
                // Reset the buffer and exit.
                this._recvBuffer = undefined;
                return;
            }
            while (true) // eslint-disable-line no-constant-condition
             {
                let nsPayload;
                try {
                    nsPayload = netstring.nsPayload(this._recvBuffer);
                }
                catch (error) {
                    logger.error('invalid netstring data received from the worker process: %s', String(error));
                    // Reset the buffer and exit.
                    this._recvBuffer = undefined;
                    return;
                }
                // Incomplete netstring message.
                if (nsPayload === -1)
                    return;
                try {
                    // We can receive JSON messages (Channel messages) or log strings.
                    switch (nsPayload[0]) {
                        // 123 = '{' (a Channel JSON messsage).
                        case 123:
                            this._processMessage(JSON.parse(nsPayload.toString('utf8')));
                            break;
                        // 68 = 'D' (a debug log).
                        case 68:
                            logger.debug(`[pid:${pid}] ${nsPayload.toString('utf8', 1)}`);
                            break;
                        // 87 = 'W' (a warn log).
                        case 87:
                            logger.warn(`[pid:${pid}] ${nsPayload.toString('utf8', 1)}`);
                            break;
                        // 69 = 'E' (an error log).
                        case 69:
                            logger.error(`[pid:${pid} ${nsPayload.toString('utf8', 1)}`);
                            break;
                        // 88 = 'X' (a dump log).
                        case 88:
                            // eslint-disable-next-line no-console
                            console.log(nsPayload.toString('utf8', 1));
                            break;
                        default:
                            // eslint-disable-next-line no-console
                            console.warn(`worker[pid:${pid}] unexpected data: %s`, nsPayload.toString('utf8', 1));
                    }
                }
                catch (error) {
                    logger.error('received invalid message from the worker process: %s', String(error));
                }
                // Remove the read payload from the buffer.
                this._recvBuffer =
                    this._recvBuffer.slice(netstring.nsLength(this._recvBuffer));
                if (!this._recvBuffer.length) {
                    this._recvBuffer = undefined;
                    return;
                }
            }
        });
        this._consumerSocket.on('end', () => (logger.debug('Consumer Channel ended by the worker process')));
        this._consumerSocket.on('error', (error) => (logger.error('Consumer Channel error: %s', String(error))));
        this._producerSocket.on('end', () => (logger.debug('Producer Channel ended by the worker process')));
        this._producerSocket.on('error', (error) => (logger.error('Producer Channel error: %s', String(error))));
    }
    /**
     * @private
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
        // Close every pending sent.
        for (const sent of this._sents.values()) {
            sent.close();
        }
        // Remove event listeners but leave a fake 'error' hander to avoid
        // propagation.
        this._consumerSocket.removeAllListeners('end');
        this._consumerSocket.removeAllListeners('error');
        this._consumerSocket.on('error', () => { });
        this._producerSocket.removeAllListeners('end');
        this._producerSocket.removeAllListeners('error');
        this._producerSocket.on('error', () => { });
        // Destroy the socket after a while to allow pending incoming messages.
        setTimeout(() => {
            try {
                this._producerSocket.destroy();
            }
            catch (error) { }
            try {
                this._consumerSocket.destroy();
            }
            catch (error) { }
        }, 200);
    }
    /**
     * @private
     */
    async request(method, internal, data) {
        this._nextId < 4294967295 ? ++this._nextId : (this._nextId = 1);
        const id = this._nextId;
        logger.debug('request() [method:%s, id:%s]', method, id);
        if (this._closed)
            throw new errors_1.InvalidStateError('Channel closed');
        const request = { id, method, internal, data };
        const ns = netstring.nsWrite(JSON.stringify(request));
        if (Buffer.byteLength(ns) > NS_MESSAGE_MAX_LEN)
            throw new Error('Channel request too big');
        // This may throw if closed or remote side ended.
        this._producerSocket.write(ns);
        return new Promise((pResolve, pReject) => {
            const timeout = 1000 * (15 + (0.1 * this._sents.size));
            const sent = {
                id: id,
                method: method,
                resolve: (data2) => {
                    if (!this._sents.delete(id))
                        return;
                    clearTimeout(sent.timer);
                    pResolve(data2);
                },
                reject: (error) => {
                    if (!this._sents.delete(id))
                        return;
                    clearTimeout(sent.timer);
                    pReject(error);
                },
                timer: setTimeout(() => {
                    if (!this._sents.delete(id))
                        return;
                    pReject(new Error('Channel request timeout'));
                }, timeout),
                close: () => {
                    clearTimeout(sent.timer);
                    pReject(new errors_1.InvalidStateError('Channel closed'));
                }
            };
            // Add sent stuff to the map.
            this._sents.set(id, sent);
        });
    }
    _processMessage(msg) {
        // If a response, retrieve its associated request.
        if (msg.id) {
            const sent = this._sents.get(msg.id);
            if (!sent) {
                logger.error('received response does not match any sent request [id:%s]', msg.id);
                return;
            }
            if (msg.accepted) {
                logger.debug('request succeeded [method:%s, id:%s]', sent.method, sent.id);
                sent.resolve(msg.data);
            }
            else if (msg.error) {
                logger.warn('request failed [method:%s, id:%s]: %s', sent.method, sent.id, msg.reason);
                switch (msg.error) {
                    case 'TypeError':
                        sent.reject(new TypeError(msg.reason));
                        break;
                    default:
                        sent.reject(new Error(msg.reason));
                }
            }
            else {
                logger.error('received response is not accepted nor rejected [method:%s, id:%s]', sent.method, sent.id);
            }
        }
        // If a notification emit it to the corresponding entity.
        else if (msg.targetId && msg.event) {
            // Due to how Promises work, it may happen that we receive a response
            // from the worker followed by a notification from the worker. If we
            // emit the notification immediately it may reach its target **before**
            // the response, destroying the ordered delivery. So we must wait a bit
            // here.
            // See https://github.com/versatica/mediasoup/issues/510
            setImmediate(() => this.emit(msg.targetId, msg.event, msg.data));
        }
        // Otherwise unexpected message.
        else {
            logger.error('received message is not a response nor a notification');
        }
    }
}
exports.Channel = Channel;
