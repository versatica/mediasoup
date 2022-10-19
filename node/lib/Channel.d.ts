import * as flatbuffers from 'flatbuffers';
import { EnhancedEventEmitter } from './EnhancedEventEmitter';
import { Response } from './fbs/fbs/response/response';
import { Body as RequestBody, Method } from './fbs/request_generated';
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
     * flatbuffer builder.
     */
    get bufferBuilder(): flatbuffers.Builder;
    /**
     * @private
     */
    close(): void;
    /**
     * @private
     */
    request(method: string, handlerId?: string, data?: any): Promise<any>;
    requestBinary(method: Method, bodyType?: RequestBody, bodyOffset?: number, handlerId?: string): Promise<Response>;
    private processMessage;
    private processBuffer;
}
//# sourceMappingURL=Channel.d.ts.map