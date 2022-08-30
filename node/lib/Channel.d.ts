import { EnhancedEventEmitter } from './EnhancedEventEmitter';
export declare class Channel extends EnhancedEventEmitter {
    #private;
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
    request(method: string, handlerId?: string, data?: any): Promise<any>;
    private processMessage;
}
//# sourceMappingURL=Channel.d.ts.map