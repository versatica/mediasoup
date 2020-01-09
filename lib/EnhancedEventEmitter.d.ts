/// <reference types="node" />
import { EventEmitter } from 'events';
import { Logger } from './Logger';
export declare class EnhancedEventEmitter extends EventEmitter {
    protected readonly _logger: Logger;
    constructor(logger?: Logger);
    safeEmit(event: string, ...args: any[]): boolean;
    safeEmitAsPromise(event: string, ...args: any[]): Promise<any>;
}
//# sourceMappingURL=EnhancedEventEmitter.d.ts.map