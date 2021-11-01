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
    notify(event: string, internal: object, data: any | undefined, payload: string | Buffer): void;
    /**
     * @private
     */
    request(method: string, internal: object, data: any, payload: string | Buffer): Promise<any>;
    private processData;
}
//# sourceMappingURL=PayloadChannel.d.ts.map