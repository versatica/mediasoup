/// <reference types="node" />
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
export declare class PayloadChannel extends EnhancedEventEmitter {
    private _closed;
    private readonly _producerSocket;
    private readonly _consumerSocket;
    private _recvBuffer?;
    private _ongoingNotification?;
    /**
     * @private
     */
    constructor({ producerSocket, consumerSocket, pid }: {
        producerSocket: any;
        consumerSocket: any;
        pid: number;
    });
    /**
     * @private
     */
    close(): void;
    /**
     * @private
     */
    send(event: string, internal: object, data: any | undefined, payload: string | Buffer): void;
    private _processMessage;
    private _processPayload;
}
//# sourceMappingURL=DeviceChannel.d.ts.map