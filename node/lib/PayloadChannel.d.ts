/// <reference types="node" />
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
export declare class PayloadChannel extends EnhancedEventEmitter {
    #private;
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
    notify(event: string, internal: string, data: string | undefined, payload: string | Buffer): void;
    /**
     * @private
     */
    request(method: string, internal: string, data: string, payload: string | Buffer): Promise<any>;
    private processData;
}
//# sourceMappingURL=PayloadChannel.d.ts.map