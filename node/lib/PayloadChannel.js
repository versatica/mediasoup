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
var _closed, _producerSocket, _consumerSocket, _nextId, _sents, _recvBuffer, _ongoingNotification;
Object.defineProperty(exports, "__esModule", { value: true });
const os = require("os");
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const errors_1 = require("./errors");
const littleEndian = os.endianness() == 'LE';
const logger = new Logger_1.Logger('PayloadChannel');
// Binary length for a 4194304 bytes payload.
const MESSAGE_MAX_LEN = 4194308;
const PAYLOAD_MAX_LEN = 4194304;
class PayloadChannel extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     */
    constructor({ producerSocket, consumerSocket }) {
        super();
        // Closed flag.
        _closed.set(this, false);
        // Unix Socket instance for sending messages to the worker process.
        _producerSocket.set(this, void 0);
        // Unix Socket instance for receiving messages to the worker process.
        _consumerSocket.set(this, void 0);
        // Next id for messages sent to the worker process.
        _nextId.set(this, 0);
        // Map of pending sent requests.
        _sents.set(this, new Map());
        // Buffer for reading messages from the worker.
        _recvBuffer.set(this, Buffer.alloc(0));
        // Ongoing notification (waiting for its payload).
        _ongoingNotification.set(this, void 0);
        logger.debug('constructor()');
        __classPrivateFieldSet(this, _producerSocket, producerSocket);
        __classPrivateFieldSet(this, _consumerSocket, consumerSocket);
        // Read PayloadChannel notifications from the worker.
        __classPrivateFieldGet(this, _consumerSocket).on('data', (buffer) => {
            if (!__classPrivateFieldGet(this, _recvBuffer).length) {
                __classPrivateFieldSet(this, _recvBuffer, buffer);
            }
            else {
                __classPrivateFieldSet(this, _recvBuffer, Buffer.concat([__classPrivateFieldGet(this, _recvBuffer), buffer], __classPrivateFieldGet(this, _recvBuffer).length + buffer.length));
            }
            if (__classPrivateFieldGet(this, _recvBuffer).length > PAYLOAD_MAX_LEN) {
                logger.error('receiving buffer is full, discarding all data in it');
                // Reset the buffer and exit.
                __classPrivateFieldSet(this, _recvBuffer, Buffer.alloc(0));
                return;
            }
            let msgStart = 0;
            while (true) // eslint-disable-line no-constant-condition
             {
                const readLen = __classPrivateFieldGet(this, _recvBuffer).length - msgStart;
                if (readLen < 4) {
                    // Incomplete data.
                    break;
                }
                const dataView = new DataView(__classPrivateFieldGet(this, _recvBuffer).buffer, __classPrivateFieldGet(this, _recvBuffer).byteOffset + msgStart);
                const msgLen = dataView.getUint32(0, littleEndian);
                if (readLen < 4 + msgLen) {
                    // Incomplete data.
                    break;
                }
                const payload = __classPrivateFieldGet(this, _recvBuffer).subarray(msgStart + 4, msgStart + 4 + msgLen);
                msgStart += 4 + msgLen;
                this.processData(payload);
            }
            if (msgStart != 0) {
                __classPrivateFieldSet(this, _recvBuffer, __classPrivateFieldGet(this, _recvBuffer).slice(msgStart));
            }
        });
        __classPrivateFieldGet(this, _consumerSocket).on('end', () => (logger.debug('Consumer PayloadChannel ended by the worker process')));
        __classPrivateFieldGet(this, _consumerSocket).on('error', (error) => (logger.error('Consumer PayloadChannel error: %s', String(error))));
        __classPrivateFieldGet(this, _producerSocket).on('end', () => (logger.debug('Producer PayloadChannel ended by the worker process')));
        __classPrivateFieldGet(this, _producerSocket).on('error', (error) => (logger.error('Producer PayloadChannel error: %s', String(error))));
    }
    /**
     * @private
     */
    close() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('close()');
        __classPrivateFieldSet(this, _closed, true);
        // Remove event listeners but leave a fake 'error' hander to avoid
        // propagation.
        __classPrivateFieldGet(this, _consumerSocket).removeAllListeners('end');
        __classPrivateFieldGet(this, _consumerSocket).removeAllListeners('error');
        __classPrivateFieldGet(this, _consumerSocket).on('error', () => { });
        __classPrivateFieldGet(this, _producerSocket).removeAllListeners('end');
        __classPrivateFieldGet(this, _producerSocket).removeAllListeners('error');
        __classPrivateFieldGet(this, _producerSocket).on('error', () => { });
        // Destroy the socket after a while to allow pending incoming messages.
        setTimeout(() => {
            try {
                __classPrivateFieldGet(this, _producerSocket).destroy();
            }
            catch (error) { }
            try {
                __classPrivateFieldGet(this, _consumerSocket).destroy();
            }
            catch (error) { }
        }, 200);
    }
    /**
     * @private
     */
    notify(event, internal, data, payload) {
        logger.debug('notify() [event:%s]', event);
        if (__classPrivateFieldGet(this, _closed))
            throw new errors_1.InvalidStateError('PayloadChannel closed');
        const notification = JSON.stringify({ event, internal, data });
        if (Buffer.byteLength(notification) > MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel notification too big');
        else if (Buffer.byteLength(payload) > MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel payload too big');
        try {
            // This may throw if closed or remote side ended.
            __classPrivateFieldGet(this, _producerSocket).write(Buffer.from(Uint32Array.of(Buffer.byteLength(notification)).buffer));
            __classPrivateFieldGet(this, _producerSocket).write(notification);
        }
        catch (error) {
            logger.warn('notify() | sending notification failed: %s', String(error));
            return;
        }
        try {
            // This may throw if closed or remote side ended.
            __classPrivateFieldGet(this, _producerSocket).write(Buffer.from(Uint32Array.of(Buffer.byteLength(payload)).buffer));
            __classPrivateFieldGet(this, _producerSocket).write(payload);
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
        __classPrivateFieldGet(this, _nextId) < 4294967295 ? __classPrivateFieldSet(this, _nextId, +__classPrivateFieldGet(this, _nextId) + 1) : (__classPrivateFieldSet(this, _nextId, 1));
        const id = __classPrivateFieldGet(this, _nextId);
        logger.debug('request() [method:%s, id:%s]', method, id);
        if (__classPrivateFieldGet(this, _closed))
            throw new errors_1.InvalidStateError('Channel closed');
        const request = JSON.stringify({ id, method, internal, data });
        if (Buffer.byteLength(request) > MESSAGE_MAX_LEN)
            throw new Error('Channel request too big');
        else if (Buffer.byteLength(payload) > MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel payload too big');
        // This may throw if closed or remote side ended.
        __classPrivateFieldGet(this, _producerSocket).write(Buffer.from(Uint32Array.of(Buffer.byteLength(request)).buffer));
        __classPrivateFieldGet(this, _producerSocket).write(request);
        __classPrivateFieldGet(this, _producerSocket).write(Buffer.from(Uint32Array.of(Buffer.byteLength(payload)).buffer));
        __classPrivateFieldGet(this, _producerSocket).write(payload);
        return new Promise((pResolve, pReject) => {
            const sent = {
                id: id,
                method: method,
                resolve: (data2) => {
                    if (!__classPrivateFieldGet(this, _sents).delete(id))
                        return;
                    pResolve(data2);
                },
                reject: (error) => {
                    if (!__classPrivateFieldGet(this, _sents).delete(id))
                        return;
                    pReject(error);
                },
                close: () => {
                    pReject(new errors_1.InvalidStateError('Channel closed'));
                }
            };
            // Add sent stuff to the map.
            __classPrivateFieldGet(this, _sents).set(id, sent);
        });
    }
    processData(data) {
        if (!__classPrivateFieldGet(this, _ongoingNotification)) {
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
                const sent = __classPrivateFieldGet(this, _sents).get(msg.id);
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
                __classPrivateFieldSet(this, _ongoingNotification, {
                    targetId: msg.targetId,
                    event: msg.event,
                    data: msg.data
                });
            }
            else {
                logger.error('received data is not a notification nor a response');
                return;
            }
        }
        else {
            const payload = data;
            // Emit the corresponding event.
            this.emit(__classPrivateFieldGet(this, _ongoingNotification).targetId, __classPrivateFieldGet(this, _ongoingNotification).event, __classPrivateFieldGet(this, _ongoingNotification).data, payload);
            // Unset ongoing notification.
            __classPrivateFieldSet(this, _ongoingNotification, undefined);
        }
    }
}
exports.PayloadChannel = PayloadChannel;
_closed = new WeakMap(), _producerSocket = new WeakMap(), _consumerSocket = new WeakMap(), _nextId = new WeakMap(), _sents = new WeakMap(), _recvBuffer = new WeakMap(), _ongoingNotification = new WeakMap();
