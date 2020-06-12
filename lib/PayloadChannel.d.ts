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
    constructor({ producerSocket, consumerSocket }: {
        producerSocket: any;
        consumerSocket: any;
    });
    /**
     * @private
     */
    close(): void;
    /**
     * @private
     */
    notify(event: string, internal: object, data: any | undefined, payload: string | Buffer): void;
    private _processData;
}
//# sourceMappingURL=PayloadChannel.d.ts.map