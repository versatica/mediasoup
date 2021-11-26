"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.PayloadChannel = void 0;
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
    // Closed flag.
    #closed = false;
    // Unix Socket instance for sending messages to the worker process.
    #producerSocket;
    // Unix Socket instance for receiving messages to the worker process.
    #consumerSocket;
    // Next id for messages sent to the worker process.
    #nextId = 0;
    // Map of pending sent requests.
    #sents = new Map();
    // Buffer for reading messages from the worker.
    #recvBuffer = Buffer.alloc(0);
    // Ongoing notification (waiting for its payload).
    #ongoingNotification;
    /**
     * @private
     */
    constructor({ producerSocket, consumerSocket }) {
        super();
        logger.debug('constructor()');
        this.#producerSocket = producerSocket;
        this.#consumerSocket = consumerSocket;
        // Read PayloadChannel notifications from the worker.
        this.#consumerSocket.on('data', (buffer) => {
            if (!this.#recvBuffer.length) {
                this.#recvBuffer = buffer;
            }
            else {
                this.#recvBuffer = Buffer.concat([this.#recvBuffer, buffer], this.#recvBuffer.length + buffer.length);
            }
            if (this.#recvBuffer.length > PAYLOAD_MAX_LEN) {
                logger.error('receiving buffer is full, discarding all data in it');
                // Reset the buffer and exit.
                this.#recvBuffer = Buffer.alloc(0);
                return;
            }
            let msgStart = 0;
            while (true) // eslint-disable-line no-constant-condition
             {
                const readLen = this.#recvBuffer.length - msgStart;
                if (readLen < 4) {
                    // Incomplete data.
                    break;
                }
                const dataView = new DataView(this.#recvBuffer.buffer, this.#recvBuffer.byteOffset + msgStart);
                const msgLen = dataView.getUint32(0, littleEndian);
                if (readLen < 4 + msgLen) {
                    // Incomplete data.
                    break;
                }
                const payload = this.#recvBuffer.subarray(msgStart + 4, msgStart + 4 + msgLen);
                msgStart += 4 + msgLen;
                this.processData(payload);
            }
            if (msgStart != 0) {
                this.#recvBuffer = this.#recvBuffer.slice(msgStart);
            }
        });
        this.#consumerSocket.on('end', () => (logger.debug('Consumer PayloadChannel ended by the worker process')));
        this.#consumerSocket.on('error', (error) => (logger.error('Consumer PayloadChannel error: %s', String(error))));
        this.#producerSocket.on('end', () => (logger.debug('Producer PayloadChannel ended by the worker process')));
        this.#producerSocket.on('error', (error) => (logger.error('Producer PayloadChannel error: %s', String(error))));
    }
    /**
     * @private
     */
    close() {
        if (this.#closed)
            return;
        logger.debug('close()');
        this.#closed = true;
        // Remove event listeners but leave a fake 'error' hander to avoid
        // propagation.
        this.#consumerSocket.removeAllListeners('end');
        this.#consumerSocket.removeAllListeners('error');
        this.#consumerSocket.on('error', () => { });
        this.#producerSocket.removeAllListeners('end');
        this.#producerSocket.removeAllListeners('error');
        this.#producerSocket.on('error', () => { });
        // Destroy the socket after a while to allow pending incoming messages.
        setTimeout(() => {
            try {
                this.#producerSocket.destroy();
            }
            catch (error) { }
            try {
                this.#consumerSocket.destroy();
            }
            catch (error) { }
        }, 200);
    }
    /**
     * @private
     */
    notify(event, internal, data, payload) {
        logger.debug('notify() [event:%s]', event);
        if (this.#closed)
            throw new errors_1.InvalidStateError('PayloadChannel closed');
        const notification = JSON.stringify({ event, internal, data });
        if (Buffer.byteLength(notification) > MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel notification too big');
        else if (Buffer.byteLength(payload) > MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel payload too big');
        try {
            // This may throw if closed or remote side ended.
            this.#producerSocket.write(Buffer.from(Uint32Array.of(Buffer.byteLength(notification)).buffer));
            this.#producerSocket.write(notification);
        }
        catch (error) {
            logger.warn('notify() | sending notification failed: %s', String(error));
            return;
        }
        try {
            // This may throw if closed or remote side ended.
            this.#producerSocket.write(Buffer.from(Uint32Array.of(Buffer.byteLength(payload)).buffer));
            this.#producerSocket.write(payload);
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
        this.#nextId < 4294967295 ? ++this.#nextId : (this.#nextId = 1);
        const id = this.#nextId;
        logger.debug('request() [method:%s, id:%s]', method, id);
        if (this.#closed)
            throw new errors_1.InvalidStateError('Channel closed');
        const request = JSON.stringify({ id, method, internal, data });
        if (Buffer.byteLength(request) > MESSAGE_MAX_LEN)
            throw new Error('Channel request too big');
        else if (Buffer.byteLength(payload) > MESSAGE_MAX_LEN)
            throw new Error('PayloadChannel payload too big');
        // This may throw if closed or remote side ended.
        this.#producerSocket.write(Buffer.from(Uint32Array.of(Buffer.byteLength(request)).buffer));
        this.#producerSocket.write(request);
        this.#producerSocket.write(Buffer.from(Uint32Array.of(Buffer.byteLength(payload)).buffer));
        this.#producerSocket.write(payload);
        return new Promise((pResolve, pReject) => {
            const sent = {
                id: id,
                method: method,
                resolve: (data2) => {
                    if (!this.#sents.delete(id))
                        return;
                    pResolve(data2);
                },
                reject: (error) => {
                    if (!this.#sents.delete(id))
                        return;
                    pReject(error);
                },
                close: () => {
                    pReject(new errors_1.InvalidStateError('Channel closed'));
                }
            };
            // Add sent stuff to the map.
            this.#sents.set(id, sent);
        });
    }
    processData(data) {
        if (!this.#ongoingNotification) {
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
                const sent = this.#sents.get(msg.id);
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
                this.#ongoingNotification =
                    {
                        targetId: String(msg.targetId),
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
            this.emit(this.#ongoingNotification.targetId, this.#ongoingNotification.event, this.#ongoingNotification.data, payload);
            // Unset ongoing notification.
            this.#ongoingNotification = undefined;
        }
    }
}
exports.PayloadChannel = PayloadChannel;
