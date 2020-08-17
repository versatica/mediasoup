"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
// @ts-ignore
const netstring = require("netstring");
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const errors_1 = require("./errors");
const logger = new Logger_1.Logger('PayloadChannel');
// netstring length for a 4194304 bytes payload.
const NS_MESSAGE_MAX_LEN = 4194313;
const NS_PAYLOAD_MAX_LEN = 4194304;
class PayloadChannel extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     */
    constructor({ producerSocket, consumerSocket }) {
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
        // Read PayloadChannel notifications from the worker.
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
                this._processData(nsPayload);
                // Remove the read payload from the buffer.
                this._recvBuffer =
                    this._recvBuffer.slice(netstring.nsLength(this._recvBuffer));
                if (!this._recvBuffer.length) {
                    this._recvBuffer = undefined;
                    return;
                }
            }
        });
        this._consumerSocket.on('end', () => (logger.debug('Consumer PayloadChannel ended by the worker process')));
        this._consumerSocket.on('error', (error) => (logger.error('Consumer PayloadChannel error: %s', String(error))));
        this._producerSocket.on('end', () => (logger.debug('Producer PayloadChannel ended by the worker process')));
        this._producerSocket.on('error', (error) => (logger.error('Producer PayloadChannel error: %s', String(error))));
    }
    /**
     * @private
     */
    close() {
        if (this._closed)
            return;
        logger.debug('close()');
        this._closed = true;
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
    notify(event, internal, data, payload) {
        logger.debug('notify() [event:%s]', event);
        if (this._closed)
            throw new errors_1.InvalidStateError('PayloadChannel closed');
        const notification = { event, internal, data };
        const ns1 = netstring.nsWrite(JSON.stringify(notification));
        const ns2 = netstring.nsWrite(payload);
        if (Buffer.byteLength(ns1) > NS_MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel notification too big');
        else if (Buffer.byteLength(ns2) > NS_MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel payload too big');
        try {
            // This may throw if closed or remote side ended.
            this._producerSocket.write(ns1);
        }
        catch (error) {
            logger.warn('notify() | sending notification failed: %s', String(error));
            return;
        }
        try {
            // This may throw if closed or remote side ended.
            this._producerSocket.write(ns2);
        }
        catch (error) {
            logger.warn('notify() | sending payload failed: %s', String(error));
            return;
        }
    }
    /**
     * @private
     */
    async request(method, internal, data, payload) {
        this._nextId < 4294967295 ? ++this._nextId : (this._nextId = 1);
        const id = this._nextId;
        logger.debug('request() [method:%s, id:%s]', method, id);
        if (this._closed)
            throw new errors_1.InvalidStateError('Channel closed');
        const request = { id, method, internal, data };
        const ns1 = netstring.nsWrite(JSON.stringify(request));
        const ns2 = netstring.nsWrite(payload);
        if (Buffer.byteLength(ns1) > NS_MESSAGE_MAX_LEN)
            throw new Error('Channel request too big');
        else if (Buffer.byteLength(ns2) > NS_MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel payload too big');
        // This may throw if closed or remote side ended.
        this._producerSocket.write(ns1);
        this._producerSocket.write(ns2);
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
    _processData(data) {
        if (!this._ongoingNotification) {
            let msg;
            try {
                msg = JSON.parse(data.toString('utf8'));
            }
            catch (error) {
                logger.error('received invalid data from the worker process: %s', String(error));
                return;
            }
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
            // If a notification, create the ongoing notification instance.
            else if (msg.targetId && msg.event) {
                this._ongoingNotification =
                    {
                        targetId: msg.targetId,
                        event: msg.event,
                        data: msg.data
                    };
            }
            else {
                logger.error('received data is not a notification nor a response');
                return;
            }
        }
        else {
            const payload = data;
            // Emit the corresponding event.
            this.emit(this._ongoingNotification.targetId, this._ongoingNotification.event, this._ongoingNotification.data, payload);
            // Unset ongoing notification.
            this._ongoingNotification = undefined;
        }
    }
}
exports.PayloadChannel = PayloadChannel;
