import { EnhancedEventEmitter } from './EnhancedEventEmitter';
export declare class Channel extends EnhancedEventEmitter {
    private _closed;
    private readonly _producerSocket;
    private readonly _consumerSocket;
    private _nextId;
    private readonly _sents;
    private _recvBuffer?;
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
    request(method: string, internal?: object, data?: any): Promise<any>;
    private _processMessage;
}
//# sourceMappingURL=Channel.d.ts.map