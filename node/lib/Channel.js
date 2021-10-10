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
var _closed, _producerSocket, _consumerSocket, _nextId, _sents, _recvBuffer;
Object.defineProperty(exports, "__esModule", { value: true });
const os = require("os");
const Logger_1 = require("./Logger");
const EnhancedEventEmitter_1 = require("./EnhancedEventEmitter");
const errors_1 = require("./errors");
const littleEndian = os.endianness() == 'LE';
const logger = new Logger_1.Logger('Channel');
// Binary length for a 4194304 bytes payload.
const MESSAGE_MAX_LEN = 4194308;
const PAYLOAD_MAX_LEN = 4194304;
class Channel extends EnhancedEventEmitter_1.EnhancedEventEmitter {
    /**
     * @private
     */
    constructor({ producerSocket, consumerSocket, pid }) {
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
        logger.debug('constructor()');
        __classPrivateFieldSet(this, _producerSocket, producerSocket);
        __classPrivateFieldSet(this, _consumerSocket, consumerSocket);
        // Read Channel responses/notifications from the worker.
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
                try {
                    // We can receive JSON messages (Channel messages) or log strings.
                    switch (payload[0]) {
                        // 123 = '{' (a Channel JSON message).
                        case 123:
                            this.processMessage(JSON.parse(payload.toString('utf8')));
                            break;
                        // 68 = 'D' (a debug log).
                        case 68:
                            logger.debug(`[pid:${pid}] ${payload.toString('utf8', 1)}`);
                            break;
                        // 87 = 'W' (a warn log).
                        case 87:
                            logger.warn(`[pid:${pid}] ${payload.toString('utf8', 1)}`);
                            break;
                        // 69 = 'E' (an error log).
                        case 69:
                            logger.error(`[pid:${pid} ${payload.toString('utf8', 1)}`);
                            break;
                        // 88 = 'X' (a dump log).
                        case 88:
                            // eslint-disable-next-line no-console
                            console.log(payload.toString('utf8', 1));
                            break;
                        default:
                            // eslint-disable-next-line no-console
                            console.warn(`worker[pid:${pid}] unexpected data: %s`, payload.toString('utf8', 1));
                    }
                }
                catch (error) {
                    logger.error('received invalid message from the worker process: %s', String(error));
                }
            }
            if (msgStart != 0) {
                __classPrivateFieldSet(this, _recvBuffer, __classPrivateFieldGet(this, _recvBuffer).slice(msgStart));
            }
        });
        __classPrivateFieldGet(this, _consumerSocket).on('end', () => (logger.debug('Consumer Channel ended by the worker process')));
        __classPrivateFieldGet(this, _consumerSocket).on('error', (error) => (logger.error('Consumer Channel error: %s', String(error))));
        __classPrivateFieldGet(this, _producerSocket).on('end', () => (logger.debug('Producer Channel ended by the worker process')));
        __classPrivateFieldGet(this, _producerSocket).on('error', (error) => (logger.error('Producer Channel error: %s', String(error))));
    }
    /**
     * @private
     */
    close() {
        if (__classPrivateFieldGet(this, _closed))
            return;
        logger.debug('close()');
        __classPrivateFieldSet(this, _closed, true);
        // Close every pending sent.
        for (const sent of __classPrivateFieldGet(this, _sents).values()) {
            sent.close();
        }
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
    async request(method, internal, data) {
        __classPrivateFieldGet(this, _nextId) < 4294967295 ? __classPrivateFieldSet(this, _nextId, +__classPrivateFieldGet(this, _nextId) + 1) : (__classPrivateFieldSet(this, _nextId, 1));
        const id = __classPrivateFieldGet(this, _nextId);
        logger.debug('request() [method:%s, id:%s]', method, id);
        if (__classPrivateFieldGet(this, _closed))
            throw new errors_1.InvalidStateError('Channel closed');
        const request = { id, method, internal, data };
        const payload = JSON.stringify(request);
        if (Buffer.byteLength(payload) > MESSAGE_MAX_LEN)
            throw new Error('Channel request too big');
        // This may throw if closed or remote side ended.
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
    processMessage(msg) {
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
_closed = new WeakMap(), _producerSocket = new WeakMap(), _consumerSocket = new WeakMap(), _nextId = new WeakMap(), _sents = new WeakMap(), _recvBuffer = new WeakMap();
